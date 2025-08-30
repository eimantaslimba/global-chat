#include "pch.h"
#include "GlobalChat.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "bakkesmod/wrappers/MMRWrapper.h"

BAKKESMOD_PLUGIN(GlobalChat, "Global Chat", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
HMODULE g_BM_ModuleHandle;

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_BM_ModuleHandle = hModule;
    }
    return TRUE;
}

/**
 * @brief Called when the plugin is first loaded. Initializes resources and sets up hooks.
 */
void GlobalChat::onLoad()
{
    _globalCvarManager = cvarManager;
    moduleHandle_ = g_BM_ModuleHandle;
    LOG("GlobalChat Plugin Loaded!");

    lastMessageTime = std::chrono::steady_clock::now() - std::chrono::seconds(2);

    // Bind the F3 key to toggle the chat window.
    cvarManager->executeCommand("bind " + TOGGLE_KEY + " \"togglemenu \\\"" + GetMenuName() + "\\\"\"");

    // Initialize WebSocket manager and define callbacks
    wsManager = std::make_unique<WSManager>();
    WSManager::Callbacks cbs;
    cbs.on_connect = [this]() { OnWSConnect(); };
    cbs.on_message = [this](std::string_view msg) { OnWSMessage(msg); };
    cbs.on_error = [this](std::string_view err) { OnWSError(err); };
    cbs.on_disconnect = [this]() { OnWSDisconnect(); };

    const std::string host = "purple-oasis-rocket-league-websocket.onrender.com";
    const std::string port = "443";
    const std::string target = "/";

    LOG("Connecting to WebSocket server at {}", host);
    wsManager->Connect(host, port, target, moduleHandle_, cbs);
}

/**
 * @brief Called when the plugin is unloaded. Cleans up resources and unbinds keys.
 */
void GlobalChat::onUnload()
{
    cvarManager->executeCommand("unbind " + TOGGLE_KEY);

    if (wsManager)
    {
        LOG("Disconnecting from WebSocket server...");
        wsManager->Disconnect();
        wsManager.reset();
    }
}

/**
 * @brief Renders the main chat window UI.
 */
void GlobalChat::RenderWindow()
{
    ImGui::Columns(2, "ChatLayout", false);
    ImGui::SetColumnWidth(0, 120.0f);

    // Left column: Channel list
    ImGui::BeginChild("Channels", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
    ImGui::Text("Channels");
    ImGui::Separator();
    if (channels.empty())
    {
        ImGui::Text("Connecting...");
    }
    else
    {
        for (const auto& channel : channels)
        {
            if (ImGui::Selectable(channel.c_str(), currentChannel == channel))
            {
                currentChannel = channel;
            }
        }
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    // Right column: Chat messages and input
    if (!currentChannel.empty())
    {
        ImGui::Text("Channel: %s", currentChannel.c_str());

        ImGui::BeginChild("Messages", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
        {
            std::lock_guard<std::mutex> lock(historyMutex);
            if (chatHistory.count(currentChannel))
            {
                const auto& messages = chatHistory.at(currentChannel);
                for (const auto& msgJson : messages)
                {
                    try
                    {
                        std::string user = msgJson.value("user", "???");
                        std::string text = msgJson.value("text", "");
                        int rankTierId = msgJson.value("highest_rank", -1);

                        RankDisplayInfo displayInfo = GetRankDisplayInfo(rankTierId);

                        // Render the colored rank tag
                        std::string tag = "[" + displayInfo.tag + "]";
                        ImGui::TextColored(displayInfo.color, "%s", tag.c_str());
                        ImGui::SameLine();

                        // Render the user's name and message
                        ImGui::TextColored(displayInfo.color, "%s:", user.c_str());
                        ImGui::SameLine();
                        ImGui::TextWrapped("%s", text.c_str());
                    }
                    catch (const std::exception& e) {
                        LOG("Error rendering message: {}", e.what());
                    }
                }
            }

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 5.0f)
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();

        // Message input and send button
        size_t messageLen = strlen(inputTextBuffer);
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastMessage = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMessageTime);
        bool isOnCooldown = timeSinceLastMessage.count() < 1000;
        bool canSendMessage = !isOnCooldown && messageLen > 0;

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##MessageInput", inputTextBuffer, sizeof(inputTextBuffer), ImGuiInputTextFlags_EnterReturnsTrue) && canSendMessage)
        {
            gameWrapper->Execute([this, channel = currentChannel, message = std::string(inputTextBuffer)](GameWrapper*) {
                this->SendChatMessage(channel, message);
                });
            memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
            ImGui::SetKeyboardFocusHere(-2);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        if (!canSendMessage) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        std::string buttonText = "Send";
        if (isOnCooldown) {
            float remaining = 1.0f - (static_cast<float>(timeSinceLastMessage.count()) / 1000.0f);
            buttonText = std::to_string(remaining).substr(0, 3) + "s";
        }

        if (ImGui::Button(buttonText.c_str()) && canSendMessage)
        {
            gameWrapper->Execute([this, channel = currentChannel, message = std::string(inputTextBuffer)](GameWrapper*) {
                this->SendChatMessage(channel, message);
                });
            memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
            ImGui::SetKeyboardFocusHere(-2);
        }

        if (!canSendMessage) {
            ImGui::PopStyleColor(3);
        }
    }
    else
    {
        ImGui::Text("No channel selected.");
        ImGui::BeginChild("Messages", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
        ImGui::Text("Waiting for server connection to populate channels...");
        ImGui::EndChild();
    }

    ImGui::Columns(1);
}

/**
 * @brief Renders the plugin's settings window in the F2 menu.
 */
void GlobalChat::RenderSettings()
{
    ImGui::TextUnformatted("Global Chat Settings");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Toggle Chat Window"))
    {
        gameWrapper->Execute([this](GameWrapper* gw) {
            cvarManager->executeCommand("togglemenu \"" + GetMenuName() + "\"");
        });
    }

    ImGui::Spacing();

    ImGui::Text("Toggle Hotkey:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.9f, 1.0f), "%s", TOGGLE_KEY.c_str());
}

/**
 * @brief Constructs and sends a chat message payload to the WebSocket server.
 * @param channel The channel to send the message to.
 * @param text The content of the message.
 */
void GlobalChat::SendChatMessage(const std::string& channel, const std::string& text)
{
    if (!wsManager || !wsManager->IsConnected()) {
        LOG("Cannot send message, not connected.");
        return;
    }

    if (!gameWrapper->IsInOnlineGame()) {
        auto playerController = gameWrapper->GetPlayerController();
        if (!playerController) {
            LOG("Cannot send message, player controller is null.");
            return;
        }
    }

    std::string playerName = gameWrapper->GetPlayerName().ToString();
    if (playerName.empty()) {
        LOG("Cannot send message, failed to get player name.");
        return;
    }

    std::string platform = gameWrapper->IsUsingEpicVersion() ? "epic" : "steam";

    int highestTier = -1;
    auto mmrWrapper = gameWrapper->GetMMRWrapper();
    auto uniqueId = gameWrapper->GetUniqueID();

    if (uniqueId.GetPlatform() != OnlinePlatform_Unknown)
    {
        const std::vector<int> playlists = { 10, 11, 13 }; // 1v1, 2v2, 3v3
        for (const auto& playlistId : playlists)
        {
            SkillRank playerRank = mmrWrapper.GetPlayerRank(uniqueId, playlistId);
            if (playerRank.Tier > highestTier)
            {
                highestTier = playerRank.Tier;
            }
        }
    }

    json messagePayload = {
        {"platform", platform},
        {"channel", channel},
        {"highest_rank", highestTier},
        {"user", playerName},
        {"text", text}
    };

    wsManager->Send(messagePayload.dump());
    LOG("Sent message to channel {}: {}", channel, text);
    lastMessageTime = std::chrono::steady_clock::now();
}

/**
 * @brief Converts a numeric rank tier ID into a displayable string and color.
 * @param tier The integer ID of the rank tier.
 * @return A struct containing the display tag (e.g., "GC1") and its associated color.
 */
GlobalChat::RankDisplayInfo GlobalChat::GetRankDisplayInfo(int tier)
{
    const ImVec4 sslColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    const ImVec4 gcColor = { 1.0f, 0.4f, 0.4f, 1.0f };
    const ImVec4 cColor = { 0.8f, 0.4f, 1.0f, 1.0f };
    const ImVec4 dColor = { 0.4f, 0.6f, 1.0f, 1.0f };
    const ImVec4 pColor = { 0.4f, 0.9f, 0.9f, 1.0f };
    const ImVec4 gColor = { 1.0f, 0.8f, 0.4f, 1.0f };
    const ImVec4 sColor = { 0.7f, 0.7f, 0.8f, 1.0f };
    const ImVec4 bColor = { 0.8f, 0.6f, 0.4f, 1.0f };
    const ImVec4 unrColor = { 0.5f, 0.5f, 0.5f, 1.0f };

    switch (tier)
    {
    case 22: case 23: case 24: case 25: return { "SSL", sslColor };
    case 21: return { "GC3", gcColor };
    case 20: return { "GC2", gcColor };
    case 19: return { "GC1", gcColor };
    case 18: return { "C3", cColor };
    case 17: return { "C2", cColor };
    case 16: return { "C1", cColor };
    case 15: return { "D3", dColor };
    case 14: return { "D2", dColor };
    case 13: return { "D1", dColor };
    case 12: return { "P3", pColor };
    case 11: return { "P2", pColor };
    case 10: return { "P1", pColor };
    case 9:  return { "G3", gColor };
    case 8:  return { "G2", gColor };
    case 7:  return { "G1", gColor };
    case 6:  return { "S3", sColor };
    case 5:  return { "S2", sColor };
    case 4:  return { "S1", sColor };
    case 3:  return { "B3", bColor };
    case 2:  return { "B2", bColor };
    case 1:  return { "B1", bColor };
    default: return { "UNR", unrColor };
    }
}

/**
 * @brief Callback executed on successful WebSocket connection.
 */
void GlobalChat::OnWSConnect()
{
    LOG("Successfully connected to WebSocket server!");
    cvarManager->executeCommand("togglemenu \"" + GetMenuName() + "\"");

}

/**
 * @brief Callback executed on WebSocket error.
 * @param error A string view of the error message.
 */
void GlobalChat::OnWSError(std::string_view error)
{
    LOG("WebSocket Error: {}", error);
}

/**
 * @brief Callback executed on WebSocket disconnection.
 */
void GlobalChat::OnWSDisconnect()
{
    LOG("Disconnected from WebSocket server.");
    channels.clear();
    chatHistory.clear();
}

/**
 * @brief Callback executed when a message is received from the WebSocket server.
 * @param message A string view of the incoming message payload.
 */
void GlobalChat::OnWSMessage(std::string_view message)
{
    std::lock_guard<std::mutex> lock(historyMutex);
    try
    {
        json receivedJson = json::parse(message);

        if (receivedJson.contains("error")) {
            LOG("Server returned an error: {}", receivedJson["error"].get<std::string>());
            return;
        }

        if (receivedJson.contains("type") && receivedJson["type"] == "all_histories")
        {
            LOG("Received all channel histories.");
            chatHistory.clear();
            channels.clear();
            json histories = receivedJson["data"];
            for (auto& [channel, messages] : histories.items())
            {
                channels.push_back(channel);
                for (const auto& msgStr : messages)
                {
                    chatHistory[channel].push_back(json::parse(msgStr.get<std::string>()));
                }
            }
            if (!channels.empty() && currentChannel.empty()) {
                currentChannel = channels[0];
            }
            return;
        }

        if (receivedJson.contains("channel") && receivedJson.contains("user"))
        {
            std::string channel = receivedJson["channel"];
            chatHistory[channel].push_back(receivedJson);
            if (chatHistory[channel].size() > 150) {
                chatHistory[channel].erase(chatHistory[channel].begin());
            }
        }
    }
    catch (const json::parse_error& e)
    {
        LOG("Failed to parse incoming JSON message: {}", e.what());
        LOG("Original message: {}", message);
    }
}