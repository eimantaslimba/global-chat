#pragma once

#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include <string>

class SettingsWindowBase : public BakkesMod::Plugin::PluginSettingsWindow
{
public:
    // Required overrides for the settings window
    std::string GetPluginName() override;
    void SetImGuiContext(uintptr_t ctx) override;
    void RenderSettings() override;
};

class PluginWindowBase : public BakkesMod::Plugin::PluginWindow
{
public:
    virtual ~PluginWindowBase() = default;

    // Required overrides for the main plugin window
    std::string GetMenuName() override;
    std::string GetMenuTitle() override;
    void SetImGuiContext(uintptr_t ctx) override;
    bool ShouldBlockInput() override;
    bool IsActiveOverlay() override;
    void OnOpen() override;
    void OnClose() override;
    void Render() override;

    // Pure virtual function to be implemented by the derived class (GlobalChat)
    virtual void RenderWindow() = 0;

protected:
    bool isWindowOpen_ = false;
    std::string menuTitle_ = "GlobalChat";
};