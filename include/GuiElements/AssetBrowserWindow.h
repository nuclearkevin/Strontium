#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Graphics/Textures.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// STL includes.
#include <filesystem>

namespace Strontium
{
  class AssetBrowserWindow : public GuiWindow
  {
  public:
    AssetBrowserWindow(EditorLayer* parentLayer);
    ~AssetBrowserWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);
  private:
    // Draw the directory tree structure.
    void drawDirectoryTree(const std::string &root = "./assets");
    void drawDirectoryNode(const std::string &path, unsigned int level);

    // Draw the file and folder icons as selectables. They're separate so
    // folders get sorted to the front.
    void drawFolders(Shared<Scene> activeScene);
    void drawFiles(Shared<Scene> activeScene);

    // Handle creation of assets.
    void createMaterial();

    std::unordered_map<std::string, Texture2D*> icons;
    std::string searched;
    std::string currentDir;

    bool loadingAsset;
    std::string loadingAssetText;
  };
}
