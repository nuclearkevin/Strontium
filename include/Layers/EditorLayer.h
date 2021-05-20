#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Layers/Layers.h"
#include "Core/Events.h"
#include "Graphics/GraphicsSystem.h"
#include "GuiElements/EnvironmentWindow.h"

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
    // The various external windows.
    EnvironmentWindow enviSettings;

    // File handler objects.
    imgui_addons::ImGuiFileBrowser fileHandler;

    // The framebuffer for the scene.
    Shared<FrameBuffer> drawBuffer;

    // Editor camera.
    Shared<Camera> editorCam;

    // These need to move to the scene class when I get around to it.
    Shared<Mesh> model;

    // This needs to move to a material class.
    Shared<Shader> program;

    // Stuff for ImGui and the GUI.
    bool showPerf;
    bool showEnvi;

    std::string logBuffer;

    ImVec2 editorSize;

    const ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoCollapse;
    const ImGuiWindowFlags logFlags = 0;
    const ImGuiWindowFlags editorFlags = ImGuiWindowFlags_NoCollapse;
  };
}
