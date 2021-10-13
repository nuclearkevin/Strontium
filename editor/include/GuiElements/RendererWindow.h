#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Shaders.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"

namespace Strontium
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
