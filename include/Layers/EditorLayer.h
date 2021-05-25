#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Core/AssetManager.h"
#include "Layers/Layers.h"
#include "Graphics/GraphicsSystem.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"
#include "GuiElements/EnvironmentWindow.h"
#include "GuiElements/SceneGraphWindow.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imguibrowser/FileBrowser/ImGuiFileBrowser.h"

namespace SciRenderer
{
  class EditorLayer : public Layer
  {
  public:
    EditorLayer();
    virtual ~EditorLayer();

    virtual void onAttach() override;
    virtual void onDetach() override;
    virtual void onImGuiRender() override;
    virtual void onEvent(Event &event) override;
    virtual void onUpdate(float dt) override;

  protected:
    // The current scene.
    Shared<Scene> currentScene;
    // The framebuffer for the scene.
    Shared<FrameBuffer> drawBuffer;
    // Editor camera.
    Shared<Camera> editorCam;

    // The various external windows.
    EnvironmentWindow enviSettings;
    SceneGraphWindow sceneSettings;
    // File handler objects.
    imgui_addons::ImGuiFileBrowser fileHandler;
    // Stuff for ImGui and the GUI.
    bool showPerf;
    bool showEnvi;
    bool showSceneGraph;
    std::string logBuffer;
    ImVec2 editorSize;

    const ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoCollapse;
    const ImGuiWindowFlags logFlags = 0;
    const ImGuiWindowFlags editorFlags = ImGuiWindowFlags_NoCollapse;
  };
}
