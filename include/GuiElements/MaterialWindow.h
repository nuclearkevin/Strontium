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
  class MaterialWindow : public GuiWindow
  {
  public:
    MaterialWindow(EditorLayer* parentLayer);
    ~MaterialWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);

    Entity getSelectedEntity() { return this->selectedEntity; }
    void setSelectedEntity(Entity newEnt) { this->selectedEntity = newEnt; }
  private:
    void drawTextureWindow(const std::string &type, const std::string &submesh,
                           bool &isOpen);
    void drawMaterialWindow(const std::string &submesh, bool &isOpen);
    void drawNewMaterialWindow(const std::string &submesh, bool &isOpen);

    // Load an asset from a drag and drop action.
    void DNDTextureTarget(Material* material, const std::string &selectedType);
    void DNDMaterialTarget(const std::string &subMesh);
    void loadDNDTextureAsset(const std::string &filepath);
    void loadDNDMaterial(const std::string &filepath, const std::string &subMesh);

    Entity selectedEntity;
    AssetHandle selectedHandle;
    std::string newMaterialName;
    FileLoadTargets fileTargets;
    FileSaveTargets fileSaveTarget;
    std::pair<Material*, std::string> selectedMatTex;
  };
}
