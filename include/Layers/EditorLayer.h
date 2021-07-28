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

    Shared<Camera> getEditorCamera() { return this->editorCam; }
    Shared<FrameBuffer> getFrontBuffer() { return this->drawBuffer; }
    Shared<Scene> getActiveScene() { return this->currentScene; }
    ImVec2& getEditorSize() { return this->editorSize; }
    Entity getSelectedEntity();
    std::string& getDNDScenePath() { return this->dndScenePath; }
  protected:
    // Handle keyboard/mouse events.
    void onKeyPressEvent(KeyPressedEvent &keyEvent);
    void onMouseEvent(MouseClickEvent &mouseEvent);

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
  };
}
