#include "pch.h"
#include "WSManager.h"
#include "resource.h"
#include <boost/asio/connect.hpp>
#include <iostream>

WSManager::WSManager() {}

WSManager::~WSManager() {
    if (IsConnected()) {
        Disconnect();
    }
}

void WSManager::Connect(const std::string& host, const std::string& port, const std::string& target, HMODULE dll_handle, Callbacks callbacks) {
    if (is_connected_) return;

    host_ = host;
    port_ = port;
    target_ = target;
    callbacks_ = std::move(callbacks);

    ioc_ = std::make_unique<net::io_context>();
    ctx_ = std::make_unique<ssl::context>(ssl::context::tlsv12_client);

    HRSRC hRes = FindResource(dll_handle, MAKEINTRESOURCE(IDR_PEM1), L"PEM");
    if (hRes) {
        HGLOBAL hResLoad = LoadResource(dll_handle, hRes);
        if (hResLoad) {
            void* pCertData = LockResource(hResLoad);
            DWORD dwCertSize = SizeofResource(dll_handle, hRes);

            boost::system::error_code ec;
            ctx_->add_certificate_authority(net::buffer(pCertData, dwCertSize), ec);
            if (ec) {
                Fail(ec, "add_certificate_authority_from_resource");
                return;
            }
        }
    }
    else {
        Fail({}, "PEM resource not found in DLL");
        return;
    }

    ctx_->set_verify_mode(ssl::verify_peer);

    resolver_ = std::make_unique<tcp::resolver>(net::make_strand(*ioc_));
    ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(net::make_strand(*ioc_), *ctx_);

    network_thread_ = std::make_unique<std::thread>(&WSManager::Run, this);
}

void WSManager::Run() {
    resolver_->async_resolve(host_, port_,
        beast::bind_front_handler(&WSManager::OnResolve, this));
    ioc_->run();
    if (callbacks_.on_disconnect) {
        callbacks_.on_disconnect();
    }
}

void WSManager::Disconnect() {
    if (!is_connected_.exchange(false)) return;

    if (ioc_) {
        net::post(*ioc_, [this]() {
            beast::error_code ec;
            if (ws_ && ws_->is_open()) {
                ws_->close(websocket::close_code::normal, ec);
            }
        });
        ioc_->stop();
    }

    if (network_thread_ && network_thread_->joinable()) {
        network_thread_->join();
    }

    network_thread_.reset();
    ws_.reset();
    resolver_.reset();
    ctx_.reset();
    ioc_.reset();
}

bool WSManager::IsConnected() const {
    return is_connected_ && ws_ && ws_->is_open();
}

void WSManager::Send(const std::string& message) {
    if (!IsConnected()) return;
    net::post(ws_->get_executor(), [this, message]() {
        write_queue_.push_back(message);
        if (write_queue_.size() == 1) {
            DoWrite();
        }
    });
}

void WSManager::OnResolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) return Fail(ec, "resolve");
    net::async_connect(beast::get_lowest_layer(*ws_), results, beast::bind_front_handler(&WSManager::OnConnect, this));
}

void WSManager::OnConnect(beast::error_code ec, const tcp::endpoint& endpoint) {
    if (ec) return Fail(ec, "connect");

    if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host_.c_str())) {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
        return Fail(ec, "set SNI");
    }

    ws_->next_layer().async_handshake(ssl::stream_base::client, beast::bind_front_handler(&WSManager::OnSslHandshake, this));
}

void WSManager::OnSslHandshake(beast::error_code ec) {
    if (ec) return Fail(ec, "ssl_handshake");
    ws_->async_handshake(host_, target_, beast::bind_front_handler(&WSManager::OnHandshake, this));
}

void WSManager::OnHandshake(beast::error_code ec) {
    if (ec) return Fail(ec, "handshake");
    is_connected_ = true;
    if (callbacks_.on_connect) callbacks_.on_connect();
    DoRead();
}

void WSManager::DoRead() {
    ws_->async_read(read_buffer_, beast::bind_front_handler(&WSManager::OnRead, this));
}

void WSManager::OnRead(beast::error_code ec, std::size_t) {
    if (ec) {
        is_connected_ = false;
        if (ec != websocket::error::closed && ec != net::error::eof) Fail(ec, "read");
        return;
    }
    if (callbacks_.on_message) callbacks_.on_message(beast::buffers_to_string(read_buffer_.data()));
    read_buffer_.consume(read_buffer_.size());
    DoRead();
}

void WSManager::DoWrite() {
    ws_->async_write(net::buffer(write_queue_.front()), beast::bind_front_handler(&WSManager::OnWrite, this));
}

void WSManager::OnWrite(beast::error_code ec, std::size_t) {
    if (ec) {
        is_connected_ = false;
        return Fail(ec, "write");
    }
    write_queue_.erase(write_queue_.begin());
    if (!write_queue_.empty()) DoWrite();
}

void WSManager::Fail(beast::error_code ec, const char* what) {
    if (callbacks_.on_error) {
        callbacks_.on_error(std::string(what) + ": " + ec.message());
    }
}
