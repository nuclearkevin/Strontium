#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "GuiElements/GuiWindow.h"

// ImGui includes.
#include "imguibrowser/FileBrowser/ImGuiFileBrowser.h"

namespace SciRenderer
{
  class FileBrowserWindow : public GuiWindow
  {
  public:
    FileBrowserWindow();
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
