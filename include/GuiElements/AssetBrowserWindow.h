#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"

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
    void drawFolders(Shared<Scene> activeScene);
    void drawFiles(Shared<Scene> activeScene);

    std::string currentDir;

    std::unordered_map<std::string, Texture2D*> icons;
  };
}
