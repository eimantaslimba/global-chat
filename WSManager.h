#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <Windows.h>

#include <string>
#include <functional>
#include <memory>
#include <string_view>
#include <atomic>
#include <thread>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class WSManager {
public:
    struct Callbacks {
        std::function<void()> on_connect;
        std::function<void(std::string_view message)> on_message;
        std::function<void(std::string_view error)> on_error;
        std::function<void()> on_disconnect;
    };

    WSManager();
    ~WSManager();

    WSManager(const WSManager&) = delete;
    WSManager& operator=(const WSManager&) = delete;

    // Correct 5-argument signature
    void Connect(const std::string& host, const std::string& port, const std::string& target, HMODULE dll_handle, Callbacks callbacks);
    void Send(const std::string& message);
    void Disconnect();
    bool IsConnected() const;

private:
    std::unique_ptr<net::io_context> ioc_;
    std::unique_ptr<ssl::context> ctx_;
    std::unique_ptr<tcp::resolver> resolver_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
    std::unique_ptr<std::thread> network_thread_;

    beast::flat_buffer read_buffer_;
    std::vector<std::string> write_queue_;
    std::string host_;
    std::string port_;
    std::string target_;
    Callbacks callbacks_;
    std::atomic<bool> is_connected_{ false };

    void Run();
    void OnResolve(beast::error_code ec, tcp::resolver::results_type results);
    void OnConnect(beast::error_code ec, const tcp::endpoint& endpoint);
    void OnSslHandshake(beast::error_code ec);
    void OnHandshake(beast::error_code ec);
    void DoRead();
    void OnRead(beast::error_code ec, std::size_t bytes_transferred);
    void DoWrite();
    void OnWrite(beast::error_code ec, std::size_t bytes_transferred);
    void Fail(beast::error_code ec, const char* what);
};