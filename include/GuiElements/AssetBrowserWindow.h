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
    void onUpdate(float dt);
    void onEvent(Event &event);

  private:
    void drawFolders();
    void drawFiles();
    void loadAssetFromFile(const std::string &name, const std::string &path);

    std::string currentDir;

    std::unordered_map<std::string, Texture2D*> icons;
  };
}
