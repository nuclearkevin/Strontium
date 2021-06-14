#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Scenes/Scene.h"

namespace SciRenderer
{
  enum class FileLoadTargets
  {
    TargetTexture, TargetModel, TargetEnvironment, TargetNone
  };

  class GuiWindow
  {
  public:
    GuiWindow();
    virtual ~GuiWindow();

    virtual void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    virtual void onUpdate(float dt, Shared<Scene> activeScene);
    virtual void onEvent(Event &event);
  private:
  };
}
