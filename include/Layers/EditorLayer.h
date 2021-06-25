#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Core/AssetManager.h"
#include "Layers/Layers.h"
#include "Graphics/GraphicsSystem.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"
#include "GuiElements/GuiWindow.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  class EditorLayer : public Layer
  {
  public:
    EditorLayer();
    virtual ~EditorLayer();

    virtual void onAttach() override;
    virtual void onDetach() override;
    virtual void onImGuiRender() override;
    virtual void onEvent(Event &event) override;
    virtual void onUpdate(float dt) override;

  protected:
    // Handle the drag and drop actions.
    void DNDTarget();
    void loadDNDAsset(const std::string &filepath);

    // Handle keyboard/mouse events.
    void onKeyPressEvent(KeyPressedEvent &keyEvent);
    void onMouseEvent(MouseClickEvent &mouseEvent);

    // Screenpicking.
    void selectEntity();

    // Gizmo UI.
    int gizmoType;
    ImVec2 gizmoSelPos;
    void manipulateEntity(Entity entity);
    void drawGizmoSelector(ImVec2 windowPos, ImVec2 windowSize);

    // The current scene.
    Shared<Scene> currentScene;
    // The framebuffer for the scene.
    Shared<FrameBuffer> drawBuffer;
    // Editor camera.
    Shared<Camera> editorCam;

    // Managing the current scene.
    FileLoadTargets loadTarget;
    FileSaveTargets saveTarget;
    std::string dndScenePath;

    // The various external windows. Kinda janky.
    // TODO: use an unordered map?
    std::vector<GuiWindow*> windows;

    // Stuff for ImGui and the GUI. TODO: Move to other windows?
    bool showPerf;
    bool showSceneGraph;
    ImVec2 editorSize;
    ImVec2 bounds[2];
  };
}
