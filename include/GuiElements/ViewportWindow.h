#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  class ViewportWindow : public GuiWindow
  {
  public:
    ViewportWindow(EditorLayer* parentLayer);
    ~ViewportWindow() = default;

    void onImGuiRender(bool &isOpen, Shared<Scene> activeScene);
    void onUpdate(float dt, Shared<Scene> activeScene);
    void onEvent(Event &event);
  private:
    // Viewport windows.
    ImVec2 bounds[2];

    // Handle keyboard/mouse events.
    void onKeyPressEvent(KeyPressedEvent &keyEvent);
    void onMouseEvent(MouseClickEvent &mouseEvent);

    // Handle the drag and drop actions.
    void DNDTarget(Shared<Scene> activeScene);
    void loadDNDAsset(const std::string &filepath, Shared<Scene> activeScene);

    // Screenpicking.
    void selectEntity();

    // Gizmo UI.
    int gizmoType;
    ImVec2 gizmoSelPos;
    void manipulateEntity(Entity entity);
    void gizmoSelectDNDPayload(int selectedGizmo);
    void drawGizmoSelector(ImVec2 windowPos, ImVec2 windowSize);
  };
}
