#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"

// ImGui includes.
#include "imguibrowser/FileBrowser/ImGuiFileBrowser.h"

namespace SciRenderer
{
  // A scene graph window for viewing a graph.
  class SceneGraphWindow : public GuiWindow
  {
  public:
    SceneGraphWindow();
    ~SceneGraphWindow();

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt);
    void onEvent(Event &event);

  private:
    // File handler objects.
    imgui_addons::ImGuiFileBrowser fileHandler;

    // Various windows to open.
    bool attachMesh;
    bool attachEnvi;
    bool propsWindow;
    bool showMaterialInfo;

    // A string buffer for the selected mesh.
    std::string selectedString;

    // Functions for UI.
    void drawEntityNode(Entity entity, Shared<Scene> activeScene);
    void drawComponentNodes(); // TODO: Implement this
    void drawPropsWindow();
    void drawMaterialWindow();
    void drawShaderInfoWindow(bool &isOpen);
    void drawMeshWindow();
    void drawEnviWindow();
    void drawTextureWindow(const std::string &type, Shared<Mesh> submesh, bool &isOpen);

    Entity selectedEntity;
  };
}
