#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"

namespace SciRenderer
{
  class MaterialWindow : public GuiWindow
  {
  public:
    MaterialWindow();
    ~MaterialWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt);
    void onEvent(Event &event);

    Entity getSelectedEntity() { return this->selectedEntity; }
    void setSelectedEntity(Entity newEnt) { this->selectedEntity = newEnt; }

  private:
    Entity selectedEntity;
    std::string selectedString;
    FileLoadTargets fileTargets;
    std::pair<Material*, std::string> selectedMatTex;

    void drawTextureWindow(const std::string &type, Shared<Mesh> submesh, bool &isOpen);
  };
}
