#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Layers/Layers.h"
#include "Core/Events.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Strontium
{
  class ImGuiLayer final : public Layer
  {
  public:
    ImGuiLayer();
    ~ImGuiLayer() = default;

    void onAttach() override;
    void onDetach() override;
    void onImGuiRender() override;
    void onEvent(Event &event) override;
    void onUpdate(float dt) override;

    void beginImGui();
    void endImGui();
  private:
    ImFont* boldFont;
    ImFont* awesomeFont;
  };
}
