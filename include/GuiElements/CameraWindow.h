#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"

namespace Strontium
{
  class CameraWindow : public GuiWindow
  {
  public:
    CameraWindow(EditorLayer* parentLayer, Shared<Camera> camera);
    ~CameraWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);

  private:
    Shared<Camera> camera;
  };
}
