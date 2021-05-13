#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Layers/Layers.h"
#include "Core/Events.h"
#include "Graphics/GraphicsSystem.h"

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
    // File handler objects.
    imgui_addons::ImGuiFileBrowser fileHandler;

    // The framebuffer for the scene.
    FrameBuffer*    drawBuffer;

    // Editor camera.
    Camera* editorCam;

    // These need to move to the scene class when I get around to it.
    EnvironmentMap* skybox;
    Mesh*           model;

    // This needs to move to a material class.
    Shader* 		    program;

    // Members for the environment map.
    bool usePBR;
    bool drawIrrad;
    bool drawFilter;
    int mapRes;

    // Stuff for ImGui and the GUI.
    bool showPerf;
    std::string logBuffer;

    const ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    const ImGuiWindowFlags logFlags = 0;

    const ImGuiWindowFlags editorFlags = ImGuiWindowFlags_NoCollapse;

  };
}
