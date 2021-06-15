#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  class AssetBrowserWindow : public GuiWindow
  {
  public:
    AssetBrowserWindow();
    ~AssetBrowserWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);

  private:
    // Draw the files and folders. They're separate so folders get sorted to the
    // front.
    void drawFolders(Shared<Scene> activeScene, float &maxCursorYPos);
    void drawFiles(Shared<Scene> activeScene, float &maxCursorYPos);

    std::string currentDir;
    ImVec2 drawCursor;

    std::unordered_map<std::string, Texture2D*> icons;
  };
}
