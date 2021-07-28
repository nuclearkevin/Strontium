#include "GuiElements/ViewportWindow.h"

// Project includes.
#include "Core/Application.h"
#include "Layers/EditorLayer.h"
#include "Serialization/YamlSerialization.h"
#include "Scenes/Components.h"

// Some math for decomposing matrix transformations.
#include "glm/gtx/matrix_decompose.hpp"

// ImGizmo goodies.
#include "imguizmo/ImGuizmo.h"

namespace SciRenderer
{
  ViewportWindow::ViewportWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , gizmoType(-1)
    , gizmoSelPos(-1.0f, -1.0f)
  { }

  void
  ViewportWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    // The editor viewport.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Editor Viewport", nullptr, ImGuiWindowFlags_NoCollapse);
    {
      auto windowPos = ImGui::GetWindowPos();
      auto windowMin = ImGui::GetWindowContentRegionMin();
      auto windowMax = ImGui::GetWindowContentRegionMax();
      windowPos.y = windowPos.y + windowMin.y;
      ImVec2 contentSize = ImVec2(windowMax.x - windowMin.x, windowMax.y - windowMin.y);

      // TODO: Move off the lighting pass framebuffer and blitz the result to
      // the editor framebuffer, after post processing of course.
      ImGui::BeginChild("EditorRender");
      {
        this->parentLayer->getEditorSize() = ImGui::GetWindowSize();
        ImVec2 cursorPos = ImGui::GetCursorPos();
        ImGui::Selectable("##editorselectable", false, ImGuiSelectableFlags_Disabled, this->parentLayer->getEditorSize());
        this->DNDTarget(activeScene);
        ImGui::SetCursorPos(cursorPos);
        auto drawBuffer = this->parentLayer->getFrontBuffer();
        ImGui::Image((ImTextureID) (unsigned long) drawBuffer->getAttachID(FBOTargetParam::Colour0),
                     this->parentLayer->getEditorSize(), ImVec2(0, 1), ImVec2(1, 0));
        this->manipulateEntity(this->parentLayer->getSelectedEntity());
        this->drawGizmoSelector(windowPos, contentSize);
      }
      ImGui::EndChild();
    }
    ImGui::PopStyleVar(3);
    ImGui::End();
  }

  void
  ViewportWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  ViewportWindow::onEvent(Event &event)
  {
    switch(event.getType())
    {
      case EventType::KeyPressedEvent:
      {
        auto keyEvent = *(static_cast<KeyPressedEvent*>(&event));
        this->onKeyPressEvent(keyEvent);
        break;
      }

      case EventType::MouseClickEvent:
      {
        auto mouseEvent = *(static_cast<MouseClickEvent*>(&event));
        this->onMouseEvent(mouseEvent);
        break;
      }
      default:
      {
        break;
      }
    }
  }

  // Handle keyboard/mouse events.
  void
  ViewportWindow::onKeyPressEvent(KeyPressedEvent &keyEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int keyCode = keyEvent.getKeyCode();

    bool camStationary = this->parentLayer->getEditorCamera()->isStationary();
    bool lControlHeld = appWindow->isKeyPressed(GLFW_KEY_LEFT_CONTROL);

    switch (keyCode)
    {
      case GLFW_KEY_Q:
      {
        // Stop using the Gizmo.
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
          this->gizmoType = -1;
        break;
      }
      case GLFW_KEY_W:
      {
        // Translate.
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
          this->gizmoType = ImGuizmo::TRANSLATE;
        break;
      }
      case GLFW_KEY_E:
      {
        // Rotate.
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
          this->gizmoType = ImGuizmo::ROTATE;
        break;
      }
      case GLFW_KEY_R:
      {
        // Scale.
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
          this->gizmoType = ImGuizmo::SCALE;
        break;
      }
    }
  }

  void
  ViewportWindow::onMouseEvent(MouseClickEvent &mouseEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int mouseCode = mouseEvent.getButton();

    bool camStationary = this->parentLayer->getEditorCamera()->isStationary();
    bool lControlHeld = appWindow->isKeyPressed(GLFW_KEY_LEFT_CONTROL);

    switch (mouseCode)
    {
      case GLFW_MOUSE_BUTTON_1:
      {
        if (lControlHeld && camStationary)
          this->selectEntity();
        break;
      }
    }
  }

  // Screenpicking.
  void
  ViewportWindow::selectEntity()
  {
    auto mousePos = ImGui::GetMousePos();
    mousePos.x -= this->bounds[0].x;
    mousePos.y -= this->bounds[0].y;

    mousePos.y = (this->parentLayer->getEditorSize()).y - mousePos.y;
    if (mousePos.x >= 0.0f && mousePos.y >= 0.0f &&
        mousePos.x < (this->parentLayer->getEditorSize()).x && mousePos.y < (this->parentLayer->getEditorSize()).y)
    {
      GLint id = this->parentLayer->getFrontBuffer()->readPixel(FBOTargetParam::Colour1, glm::vec2(mousePos.x, mousePos.y)) - 1;

      EventDispatcher* dispatcher = EventDispatcher::getInstance();
      dispatcher->queueEvent(new EntitySwapEvent(id, this->parentLayer->getActiveScene().get()));
    }
  }

  // Gizmo UI.
  void
  ViewportWindow::manipulateEntity(Entity entity)
  {
    // Get the camera matrices.
    auto& camProjection = this->parentLayer->getEditorCamera()->getProjMatrix();
    auto& camView = this->parentLayer->getEditorCamera()->getViewMatrix();

    // ImGuizmo boilerplate. Prepare the drawing context and set the window to
    // draw the gizmos to.
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    auto windowMin = ImGui::GetWindowContentRegionMin();
    auto windowMax = ImGui::GetWindowContentRegionMax();
    auto windowOffset = ImGui::GetWindowPos();
    this->bounds[0] = ImVec2(windowMin.x + windowOffset.x,
                             windowMin.y + windowOffset.y);
    this->bounds[1] = ImVec2(windowMax.x + windowOffset.x,
                             windowMax.y + windowOffset.y);

    ImGuizmo::SetRect(bounds[0].x, bounds[0].y, bounds[1].x - bounds[0].x,
                      bounds[1].y - bounds[0].y);

    // Quit early if the entity is invalid.
    if (!entity)
      return;

    // Only draw the gizmo if the entity has a transform component and if a gizmo is selected.
    if (entity.hasComponent<TransformComponent>() && this->gizmoType != -1)
    {
      // Fetch the transform component.
      auto& transform = entity.getComponent<TransformComponent>();
      glm::mat4 transformMatrix = transform;

      // Manipulate the matrix. TODO: Add snapping.
      ImGuizmo::Manipulate(glm::value_ptr(camView), glm::value_ptr(camProjection),
                           (ImGuizmo::OPERATION) this->gizmoType,
                           ImGuizmo::WORLD, glm::value_ptr(transformMatrix),
                           nullptr, nullptr);

      if (ImGuizmo::IsUsing())
      {
        glm::vec3 translation, scale, skew;
        glm::vec4 perspective;
        glm::quat rotation;
        glm::decompose(transformMatrix, scale, rotation, translation, skew, perspective);

        transform.translation = translation;
        transform.rotation = glm::eulerAngles(rotation);
        transform.scale = scale;
      }
    }
  }

  void
  ViewportWindow::gizmoSelectDNDPayload(int selectedGizmo)
  {
    if (ImGui::BeginDragDropSource())
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
      if (selectedGizmo == -1)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("None");
        ImGui::PopStyleVar();
      }
      else
        ImGui::Button("None");

      ImGui::SameLine();
      if (selectedGizmo == ImGuizmo::TRANSLATE)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Translate");
        ImGui::PopStyleVar();
      }
      else
        ImGui::Button("Translate");

      ImGui::SameLine();
      if (selectedGizmo == ImGuizmo::ROTATE)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Rotate");
        ImGui::PopStyleVar();
      }
      else
        ImGui::Button("Rotate");

      ImGui::SameLine();
      if (selectedGizmo == ImGuizmo::SCALE)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Scale");
        ImGui::PopStyleVar();
      }
      else
        ImGui::Button("Scale");
      ImGui::PopStyleVar();

      int temp = 0;
      ImGui::SetDragDropPayload("GIZMO_SELECT", &temp, sizeof(int));

      ImGui::EndDragDropSource();
    }
  }

  void
  ViewportWindow::drawGizmoSelector(ImVec2 windowPos, ImVec2 windowSize)
  {
    auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;
    const ImVec2 selectorSize = ImVec2(219.0f, 18.0f);

    // Bounding box detection to make sure this thing doesn't leave the editor
    // viewport.
    this->gizmoSelPos.x = this->gizmoSelPos.x < windowPos.x ? windowPos.x : this->gizmoSelPos.x;
    this->gizmoSelPos.y = this->gizmoSelPos.y < windowPos.y ? windowPos.y : this->gizmoSelPos.y;
    this->gizmoSelPos.x = this->gizmoSelPos.x + selectorSize.x > windowPos.x + windowSize.x
      ? windowPos.x + windowSize.x - selectorSize.x : this->gizmoSelPos.x;
    this->gizmoSelPos.y = this->gizmoSelPos.y + selectorSize.y > windowPos.y + windowSize.y
      ? windowPos.y + windowSize.y - selectorSize.y : this->gizmoSelPos.y;

    ImGui::SetNextWindowPos(this->gizmoSelPos);
    ImGui::Begin("GizmoSelector", nullptr, flags);

    auto cursorPos = ImGui::GetCursorPos();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.0f);
    ImGui::Selectable("##editorselectable", false, ImGuiSelectableFlags_AllowItemOverlap, selectorSize);
    ImGui::PopStyleVar();
    gizmoSelectDNDPayload(this->gizmoType);

    ImGui::SetCursorPos(cursorPos);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    if (this->gizmoType == -1)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("None");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
      if (ImGui::Button("None"))
        this->gizmoType = -1;
    gizmoSelectDNDPayload(this->gizmoType);

    ImGui::SameLine();
    if (this->gizmoType == ImGuizmo::TRANSLATE)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("Translate");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
      if (ImGui::Button("Translate"))
        this->gizmoType = ImGuizmo::TRANSLATE;
    gizmoSelectDNDPayload(this->gizmoType);

    ImGui::SameLine();
    if (this->gizmoType == ImGuizmo::ROTATE)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("Rotate");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
      if (ImGui::Button("Rotate"))
        this->gizmoType = ImGuizmo::ROTATE;
    gizmoSelectDNDPayload(this->gizmoType);

    ImGui::SameLine();
    if (this->gizmoType == ImGuizmo::SCALE)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("Scale");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
      if (ImGui::Button("Scale"))
        this->gizmoType = ImGuizmo::SCALE;
    gizmoSelectDNDPayload(this->gizmoType);
    ImGui::PopStyleVar();

    ImGui::End();

    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GIZMO_SELECT", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        gizmoSelPos = ImGui::GetMousePos();
      ImGui::EndDragDropTarget();
    }
  }

  // Handle the drag and drop actions.
  void
  ViewportWindow::DNDTarget(Shared<Scene> activeScene)
  {
    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
      {
        this->loadDNDAsset((char*) payload->Data, activeScene);
      }
      ImGui::EndDragDropTarget();
    }
  }

  void
  ViewportWindow::loadDNDAsset(const std::string &filepath, Shared<Scene> activeScene)
  {
    std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
    std::string filetype = filename.substr(filename.find_last_of('.'));

    // If its a supported model file, load it as a new entity in the scene.
    if (filetype == ".obj" || filetype == ".FBX" || filetype == ".fbx")
    {
      auto modelAssets = AssetManager<Model>::getManager();

      auto model = activeScene->createEntity(filename.substr(0, filename.find_last_of('.')));
      model.addComponent<TransformComponent>();
      auto& rComponent = model.addComponent<RenderableComponent>(filename);
      Model::asyncLoadModel(filepath, filename, &rComponent.materials);
    }

    // If its a supported image, load and cache it.
    if (filetype == ".jpg" || filetype == ".tga" || filetype == ".png")
    {
      Texture2D::loadImageAsync(filepath);
    }

    if (filetype == ".hdr")
    {
      auto storage = Renderer3D::getStorage();
      auto ambient = activeScene->createEntity("New Ambient Component");

      storage->currentEnvironment->unloadEnvironment();
      ambient.addComponent<AmbientComponent>(filepath);
    }

    // Load a SciRender scene file. TODO: Update this with a file load event.
    if (filetype == ".srn")
      this->parentLayer->getDNDScenePath() = filepath;

    // Load a SciRender prefab object.
    if (filetype == ".sfab")
      YAMLSerialization::deserializePrefab(activeScene, filepath);
  }
}
