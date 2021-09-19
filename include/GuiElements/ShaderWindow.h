#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Graphics/Shaders.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"
#include "GuiElements/Styles.h"

namespace Strontium
{
  class ShaderWindow : public GuiWindow
  {
  public:
    ShaderWindow(EditorLayer* parentLayer);
    ~ShaderWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);

  private:
    Shader* selectedShader;
    std::string shaderName;
  };
}
