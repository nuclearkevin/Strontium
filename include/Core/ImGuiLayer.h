#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/Layers.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

namespace SciRenderer
{
  class ImGuiLayer : public Layer
  {
  public:
    ImGuiLayer();
    ~ImGuiLayer() = default;

    virtual void onAttach() override;
    virtual void onDetach() override;
    virtual void onEvent() override;

    void beginImGui();
    void endImGui();
  };
}
