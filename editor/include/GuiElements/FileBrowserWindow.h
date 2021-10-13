#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "GuiElements/GuiWindow.h"

// ImGui includes.
#include "imguibrowser/FileBrowser/ImGuiFileBrowser.h"

namespace Strontium
{
  class FileBrowserWindow : public GuiWindow
  {
  public:
    FileBrowserWindow(EditorLayer* parentLayer);
    ~FileBrowserWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);

  private:
    imgui_addons::ImGuiFileBrowser fileHandler;

    bool isOpen;

    std::string format;
    imgui_addons::ImGuiFileBrowser::DialogMode mode;
  };
}
