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
  class CameraWindow : public GuiWindow
  {
  public:
    CameraWindow(Shared<Camera> camera);
    ~CameraWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt);
    void onEvent(Event &event);

  private:
    Shared<Camera> camera;
  };
}
