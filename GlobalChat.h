#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"
#include "WSManager.h"

#include "json.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


using json = nlohmann::json;

extern HMODULE g_BM_ModuleHandle;


class GlobalChat : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase, public PluginWindowBase
{
public:
    // BakkesMod Plugin Overrides
    void onLoad() override;
    void onUnload() override;

    // GuiBase Overrides
    void RenderWindow() override;
    void RenderSettings() override;

private:
    // WebSocket Communication
    void OnWSConnect();
    void OnWSMessage(std::string_view message);
    void OnWSError(std::string_view error);
    void OnWSDisconnect();
    void SendChatMessage(const std::string& channel, const std::string& text);
    std::unique_ptr<WSManager> wsManager;

    // Rank Display
    struct RankDisplayInfo {
        std::string tag;
        ImVec4 color;
    };
    RankDisplayInfo GetRankDisplayInfo(int tier);

    // UI State & Data
    std::string currentChannel;
    std::vector<std::string> channels;
    std::map<std::string, std::vector<json>> chatHistory;
    std::mutex historyMutex;
    char inputTextBuffer[256]{};
    std::chrono::steady_clock::time_point lastMessageTime;
    HMODULE moduleHandle_ = nullptr;

    const std::string TOGGLE_KEY = "F3";
};