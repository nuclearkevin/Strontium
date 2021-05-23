#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imguibrowser/FileBrowser/ImGuiFileBrowser.h"

namespace SciRenderer
{
  // A scene graph window for viewing a graph.
  class SceneGraphWindow
  {
  public:
    SceneGraphWindow() = default;
    ~SceneGraphWindow() = default;

    void onAttach();
    void onDetach();
    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt);
    void onEvent(Event &event);

  private:
    // File handler objects.
    imgui_addons::ImGuiFileBrowser fileHandler;

    bool openModel;

    // Functions for UI.
    void drawEntityNode(Entity entity, Shared<Scene> activeScene);
    void drawComponentNodes();

    Entity selectedEntity;
  };
}
