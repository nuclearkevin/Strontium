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
  private:
    ImFont* boldFont;
    ImFont* awesomeFont;
  };
}
