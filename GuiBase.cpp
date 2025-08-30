#include "pch.h"
#include "GuiBase.h"

// -- SettingsWindowBase Implementation --

std::string SettingsWindowBase::GetPluginName()
{
    return "GlobalChat";
}

void SettingsWindowBase::SetImGuiContext(uintptr_t ctx)
{
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

void SettingsWindowBase::RenderSettings()
{
    ImGui::TextUnformatted("This is the base settings window.");
}


// -- PluginWindowBase Implementation --

std::string PluginWindowBase::GetMenuName()
{
    return "GlobalChat";
}

std::string PluginWindowBase::GetMenuTitle()
{
    return menuTitle_;
}

void PluginWindowBase::SetImGuiContext(uintptr_t ctx)
{
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool PluginWindowBase::ShouldBlockInput()
{
    return isWindowOpen_ && (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard);
}

bool PluginWindowBase::IsActiveOverlay()
{
    return isWindowOpen_;
}

void PluginWindowBase::OnOpen()
{
    isWindowOpen_ = true;
}

void PluginWindowBase::OnClose()
{
    isWindowOpen_ = false;
}

void PluginWindowBase::Render()
{
    // Do not render if the window is not open
    if (!isWindowOpen_)
    {
        return;
    }

    // Set window flags to prevent collapsing, which can be confusing for users.
    // ImGui::Begin will set isWindowOpen_ to false if the user clicks the 'X' button.
    if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    // Call the derived class's implementation of the window content
    RenderWindow();

    ImGui::End();
}