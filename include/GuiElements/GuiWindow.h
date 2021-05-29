#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Scenes/Scene.h"

namespace SciRenderer
{
  class GuiWindow
  {
  public:
    GuiWindow();
    virtual ~GuiWindow();

    virtual void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    virtual void onUpdate(float dt);
    virtual void onEvent(Event &event);
  private:

  };
}
