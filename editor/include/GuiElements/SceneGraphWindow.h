#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/EditorCamera.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Textures.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"

#include "GuiElements/GuiWindow.h"
#include "GuiElements/MaterialSubWindow.h"
#include "Widgets/LightWidgets.h"

namespace Strontium
{
  // A scene graph window for viewing a graph.
  class SceneGraphWindow : public GuiWindow
  {
  public:
    SceneGraphWindow(EditorLayer* parentLayer);
    ~SceneGraphWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);

    Entity& getSelectedEntity() { return this->selectedEntity; }
    void setSelectedEntity(Entity newEntity) { this->selectedEntity = newEntity; }

  private:
    // Functions for UI.
    void drawEntityNode(Entity entity, Shared<Scene> activeScene);
    void drawComponentNodes(Entity entity, Shared<Scene> activeScene);
    void drawPropsWindow(bool &isOpen, Shared<Scene> activeScene);

    // Load an asset from a drag and drop action.
    void loadDNDAsset();
    void loadDNDAsset(const std::string &submeshName);

    // Other subwindows.
    MaterialSubWindow materialEditor;

    // Items for a widget which makes modifying a directional light easier.
    LightWidget directionalWidget;
    float widgetWidth;

    // Various buffers and selections.
    Entity selectedEntity;
    Mesh* selectedSubmesh;
    std::string selectedString;
    FileLoadTargets fileTargets;
    FileSaveTargets saveTargets;
  };
}
