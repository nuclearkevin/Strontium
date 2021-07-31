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

    void drawDirectionalWidget();

    // TODO: Consider moving this to a separate window.
    void drawMeshWindow(bool &isOpen);

    // Load an asset from a drag and drop action.
    void loadDNDAsset(const std::string &filepath);

    // Items for a widget which makes modifying a directional light easier.
    Shared<FrameBuffer> dirBuffer;
    Model sphere;
    Shader dirWidgetShader;
    float widgetWidth;

    // Various buffers and selections.
    Entity selectedEntity;
    std::string selectedString;
    FileLoadTargets fileTargets;
    FileSaveTargets saveTargets;
  };
}
