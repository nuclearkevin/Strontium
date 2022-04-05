#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"
#include "Scenes/Components.h"

namespace Strontium
{
  class ModelWindow : public GuiWindow
  {
  public:
    ModelWindow(EditorLayer* parentLayer, bool isOpen = true);
    ~ModelWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);

    Entity getSelectedEntity() { return this->selectedEntity; }
    void setSelectedEntity(Entity newEnt) { this->selectedEntity = newEnt; }
  private:
    Entity selectedEntity;
    Asset::Handle selectedHandle;
    std::string newMaterialName;
    std::string searched;
    FileLoadTargets fileLoadTargets;
    FileSaveTargets fileSaveTarget;

    SceneNode* selectedNode;
    AnimationNode* selectedAniNode;
    Mesh* selectedSubMesh;
  };
}
