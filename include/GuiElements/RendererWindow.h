#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"

namespace SciRenderer
{
  // A window for displaying and modifying renderer settings.
  class RendererWindow : public GuiWindow
  {
  public:
    RendererWindow(EditorLayer* parentLayer);
    ~RendererWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);

  private:

  };
}
