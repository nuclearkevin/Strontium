#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/EditorCamera.h"
#include "Layers/Layers.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"

#include "GuiElements/GuiWindow.h"
#include "GuiElements/WindowManager.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Strontium
{
  enum class SceneState
  {
    Edit = 0,
    Simulate = 1,
    Play = 2
  };

  class EditorLayer final : public Layer
  {
  public:
    EditorLayer();
    virtual ~EditorLayer() override;

    virtual void onAttach() override;
    virtual void onDetach() override;
    virtual void onImGuiRender() override;
    virtual void onEvent(Event &event) override;
    virtual void onUpdate(float dt) override;

    EditorCamera& getEditorCamera() { return this->editorCam; }
    FrameBuffer& getFrontBuffer() { return this->drawBuffer; }
    Shared<Scene> getActiveScene() { return this->currentScene; }
    ImVec2& getEditorSize() { return this->editorSize; }
    Entity getSelectedEntity();
    SceneState getSceneState() { return this->sceneState; }
    std::string& getDNDScenePath() { return this->dndScenePath; }
  protected:
    // Handle keyboard/mouse events.
    void onKeyPressEvent(KeyPressedEvent &keyEvent);
    void onMouseEvent(MouseClickEvent &mouseEvent);

    void onScenePlay();
    void onSceneSimulate();
    void onSceneStop();

    // The current scene.
    Shared<Scene> currentScene;
    // Backup of the current scene so we can revert to a version before runtime changes.
    Shared<Scene> backupScene;

    // The framebuffer for the scene.
    FrameBuffer drawBuffer;
    // Editor camera.
    EditorCamera editorCam;

    // Managing the current scene.
    SceneState sceneState;
    FileLoadTargets loadTarget;
    FileSaveTargets saveTarget;
    std::string dndScenePath;

    // Various external windows.
    WindowManager windowManager;

    // Stuff for ImGui and the GUI. TODO: Move to other windows?
    bool showPerf;
    ImVec2 editorSize;
  };
}
