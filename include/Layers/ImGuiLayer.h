#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Layers/Layers.h"
#include "Core/Events.h"

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
    virtual void onEvent(Event &event) override;

    void beginImGui();
    void endImGui();
  };
}
