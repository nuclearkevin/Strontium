#include "GuiElements/ViewportWindow.h"

// Project includes.
#include "Core/Application.h"
#include "Core/KeyCodes.h"
#include "EditorLayer.h"
#include "Scenes/Components.h"
#include "GuiElements/Styles.h"
#include "Serialization/YamlSerialization.h"
#include "Utils/AsyncAssetLoading.h"

// Some math for decomposing matrix transformations.
#include "glm/gtx/matrix_decompose.hpp"

// ImGizmo goodies.
#include "imguizmo/ImGuizmo.h"

namespace Strontium
{
  ViewportWindow::ViewportWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , gizmoType(-1)
    , gizmoSelPos(-1.0f, -1.0f)
    , selectorSize(0.0f, 0.0f)
  { }

  void
  ViewportWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    // The editor viewport.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(ICON_FA_GAMEPAD"  Editor Viewport", nullptr, ImGuiWindowFlags_NoCollapse);
    {
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
        this->drawGizmoSelector();
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
      case EventType::WindowResizeEvent:
      {

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

    bool camStationary = this->parentLayer->getEditorCamera().isStationary();
    bool lControlHeld = appWindow->isKeyPressed(SR_KEY_LEFT_CONTROL);

    switch (keyCode)
    {
      case SR_KEY_Q:
      {
        // Stop using the Gizmo.
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
          this->gizmoType = -1;
        break;
      }
      case SR_KEY_W:
      {
        // Translate.
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
          this->gizmoType = ImGuizmo::TRANSLATE;
        break;
      }
      case SR_KEY_E:
      {
        // Rotate.
        if (lControlHeld && keyEvent.getRepeatCount() == 0 && camStationary)
          this->gizmoType = ImGuizmo::ROTATE;
        break;
      }
      case SR_KEY_R:
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

    bool camStationary = this->parentLayer->getEditorCamera().isStationary();
    bool lControlHeld = appWindow->isKeyPressed(SR_KEY_LEFT_CONTROL);

    switch (mouseCode)
    {
      case SR_MOUSE_BUTTON_1:
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
      int id = this->parentLayer->getFrontBuffer()->readPixel(FBOTargetParam::Colour1, glm::vec2(mousePos.x, mousePos.y)) - 1;

      EventDispatcher* dispatcher = EventDispatcher::getInstance();
      dispatcher->queueEvent(new EntitySwapEvent(id, this->parentLayer->getActiveScene().get()));
    }
  }

  // Gizmo UI.
  void
  ViewportWindow::manipulateEntity(Entity entity)
  {
    auto windowMin = ImGui::GetWindowContentRegionMin();
    auto windowMax = ImGui::GetWindowContentRegionMax();
    auto windowOffset = ImGui::GetWindowPos();
    this->bounds[0] = ImVec2(windowMin.x + windowOffset.x,
        windowMin.y + windowOffset.y);
    this->bounds[1] = ImVec2(windowMax.x + windowOffset.x,
        windowMax.y + windowOffset.y);

    if (this->parentLayer->getSceneState() != SceneState::Edit)
      return;

    // Get the camera matrices.
    auto& camProjection = this->parentLayer->getEditorCamera().getProjMatrix();
    auto& camView = this->parentLayer->getEditorCamera().getViewMatrix();

    // Quit early if the entity is invalid.
    if (!entity)
      return;

    // ImGuizmo boilerplate. Prepare the drawing context and set the window to
    // draw the gizmos to.
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    ImGuizmo::SetRect(bounds[0].x, bounds[0].y, bounds[1].x - bounds[0].x,
                      bounds[1].y - bounds[0].y);

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
        ImGui::Button(ICON_FA_MOUSE_POINTER);
        ImGui::PopStyleVar();
      }
      else
        ImGui::Button(ICON_FA_MOUSE_POINTER);

      ImGui::SameLine();
      if (selectedGizmo == ImGuizmo::TRANSLATE)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button(ICON_FA_ARROWS);
        ImGui::PopStyleVar();
      }
      else
        ImGui::Button(ICON_FA_ARROWS);

      ImGui::SameLine();
      if (selectedGizmo == ImGuizmo::ROTATE)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button(ICON_FA_UNDO);
        ImGui::PopStyleVar();
      }
      else
        ImGui::Button(ICON_FA_UNDO);

      ImGui::SameLine();
      if (selectedGizmo == ImGuizmo::SCALE)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button(ICON_FA_EXPAND);
        ImGui::PopStyleVar();
      }
      else
        ImGui::Button(ICON_FA_EXPAND);
      ImGui::PopStyleVar();

      int temp = 0;
      ImGui::SetDragDropPayload("GIZMO_SELECT", &temp, sizeof(int));

      ImGui::EndDragDropSource();
    }
  }

  void
  ViewportWindow::drawGizmoSelector()
  {
    auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking
               | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize;

    // Bounding box detection to make sure this thing doesn't leave the editor
    // viewport.
    /*
    this->gizmoSelPos.x = this->gizmoSelPos.x < this->bounds[0].x ? this->bounds[0].x : this->gizmoSelPos.x;
    this->gizmoSelPos.y = this->gizmoSelPos.y < this->bounds[0].y ? this->bounds[0].y : this->gizmoSelPos.y;
    this->gizmoSelPos.x = this->gizmoSelPos.x > this->bounds[1].x ? this->bounds[1].x : this->gizmoSelPos.x;
    this->gizmoSelPos.y = this->gizmoSelPos.y > this->bounds[1].y ? this->bounds[1].y : this->gizmoSelPos.y;
    */

    auto windowPos = ImGui::GetWindowPos();
    auto windowSize = ImVec2(bounds[1].x - bounds[0].x, bounds[1].y - bounds[0].y);
    this->gizmoSelPos.x = this->gizmoSelPos.x < windowPos.x ? windowPos.x : this->gizmoSelPos.x;
    this->gizmoSelPos.y = this->gizmoSelPos.y < windowPos.y ? windowPos.y : this->gizmoSelPos.y;
    this->gizmoSelPos.x = this->gizmoSelPos.x + 100 > windowPos.x + windowSize.x ? windowPos.x + windowSize.x - 100 : this->gizmoSelPos.x;
    this->gizmoSelPos.y = this->gizmoSelPos.y + 18 > windowPos.y + windowSize.y ? windowPos.y + windowSize.y - 20 : this->gizmoSelPos.y;

    ImGui::SetNextWindowPos(this->gizmoSelPos);
    ImGui::SetNextWindowSize(ImVec2(100, 18));
    ImGui::Begin("GizmoSelector", nullptr, flags);
    this->selectorSize = ImGui::GetWindowSize();

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
      ImGui::Button(ICON_FA_MOUSE_POINTER);
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
      if (ImGui::Button(ICON_FA_MOUSE_POINTER))
        this->gizmoType = -1;
    gizmoSelectDNDPayload(this->gizmoType);

    ImGui::SameLine();
    if (this->gizmoType == ImGuizmo::TRANSLATE)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button(ICON_FA_ARROWS);
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
      if (ImGui::Button(ICON_FA_ARROWS))
        this->gizmoType = ImGuizmo::TRANSLATE;
    gizmoSelectDNDPayload(this->gizmoType);

    ImGui::SameLine();
    if (this->gizmoType == ImGuizmo::ROTATE)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button(ICON_FA_UNDO);
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
      if (ImGui::Button(ICON_FA_UNDO))
        this->gizmoType = ImGuizmo::ROTATE;
    gizmoSelectDNDPayload(this->gizmoType);

    ImGui::SameLine();
    if (this->gizmoType == ImGuizmo::SCALE)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button(ICON_FA_EXPAND);
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    else
      if (ImGui::Button(ICON_FA_EXPAND))
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
#ifdef WIN32
    std::string filename = filepath.substr(filepath.find_last_of('\\') + 1);
    std::string filetype = filename.substr(filename.find_last_of('.'));
#else
    std::string filename = filepath.substr(filepath.find_last_of('\\') + 1);
    std::string filetype = filename.substr(filename.find_last_of('.'));
#endif

    // If its a supported model file, load it as a new entity in the scene.
    if (filetype == ".obj" || filetype == ".FBX" || filetype == ".fbx"
        || filetype == ".blend" || filetype == ".gltf" || filetype == ".glb"
        || filetype == ".dae")
    {
      auto modelAssets = AssetManager<Model>::getManager();

      auto model = activeScene->createEntity(filename.substr(0, filename.find_last_of('.')));
      model.addComponent<TransformComponent>();
      auto& rComponent = model.addComponent<RenderableComponent>(filename);
      AsyncLoading::asyncLoadModel(filepath, filename, model, activeScene.get());
    }

    // If its a supported image, load and cache it.
    if (filetype == ".jpg" || filetype == ".tga" || filetype == ".png")
      AsyncLoading::loadImageAsync(filepath);

    if (filetype == ".hdr")
    {
      auto storage = Renderer3D::getStorage();
      auto ambient = activeScene->createEntity("New Ambient Component");

      storage->currentEnvironment->unloadEnvironment();
      auto environment = ambient.addComponent<AmbientComponent>(filepath).ambient;
      auto state = Renderer3D::getState();

      environment->equiToCubeMap(true, state->skyboxWidth, state->skyboxWidth);
      environment->precomputeIrradiance(state->irradianceWidth, state->irradianceWidth, true);
      environment->precomputeSpecular(state->prefilterWidth, state->prefilterWidth, true);
    }

    // Load a SciRender scene file. TODO: Update this with a file load event.
    if (filetype == ".srn")
      this->parentLayer->getDNDScenePath() = filepath;

    // Load a SciRender prefab object.
    if (filetype == ".sfab")
      YAMLSerialization::deserializePrefab(activeScene, filepath);
  }
}
