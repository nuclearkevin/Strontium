#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
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
  class EnvironmentWindow
  {
  public:
    EnvironmentWindow();
    ~EnvironmentWindow();

    void onAttach();
    void onDetach();
    void onImGuiRender();
    void onUpdate(float dt);
    void onEvent(Event &event);

    // Get the environment map.
    inline Shared<EnvironmentMap> getEnvironmentMap() { return this->skybox; }
  protected:
    // File handler objects.
    imgui_addons::ImGuiFileBrowser fileHandler;

    // The environment map.
    Shared<EnvironmentMap> skybox;

    // Members for the environment map.
    bool usePBR;
    bool drawIrrad;
    bool drawFilter;
    int mapRes;
  };
}
