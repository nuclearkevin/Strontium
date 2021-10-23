#include "GuiElements/SceneGraphWindow.h"

// STL includes.
#include <cstring>

// Project includes.
#include "Core/AssetManager.h"
#include "Graphics/Renderer.h"
#include "GuiElements/Styles.h"
#include "Scenes/Components.h"
#include "Serialization/YamlSerialization.h"
#include "Utils/AsyncAssetLoading.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// Some math for decomposing matrix transformations.
#include "glm/gtx/matrix_decompose.hpp"

// ImGizmo goodies.
#include "imguizmo/ImGuizmo.h"

namespace Strontium
{
  // Templated helper functions for components.
  //----------------------------------------------------------------------------
  // Draw the component's 'properties' gui elements.
  template <typename T, typename Function>
  static void drawComponentProperties(const std::string &name, Entity parent, Function ui)
  {
    if (!parent)
      return;

    if (parent.hasComponent<T>())
    {
      auto& component = parent.getComponent<T>();
      if (ImGui::CollapsingHeader(name.c_str()))
      {
        ui(component);
      }
    }
  }

  // Draw the component's 'add' gui elements.
  template <typename T, typename ... Args>
  static void drawComponentAdd(const std::string &name, Entity parent,
                               Args ... args)
  {
    if (!parent)
      return;

    if (!parent.hasComponent<T>())
    {
      if (ImGui::MenuItem(name.c_str()))
        parent.addComponent<T>(std::forward<Args>(args)...);
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::MenuItem(name.c_str());
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
  }

  // Draw the component's 'remove' gui elements.
  template <typename T>
  static void drawComponentRemove(const std::string &name, Entity parent)
  {
    if (!parent)
      return;

    if (parent.hasComponent<T>())
    {
      if (ImGui::MenuItem(name.c_str()))
        parent.removeComponent<T>();
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::MenuItem(name.c_str());
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
  }

  // Copy a component between a source and a target.
  template <typename T>
  static void copyComponent(Entity source, Entity target)
  {
    if (!(source && target))
      return;

    if (source.hasComponent<T>())
    {
      if (target.hasComponent<T>())
      {
        auto& comp = target.getComponent<T>();
        auto temp = source.getComponent<T>();
        comp = temp;
      }
      else
      {
        auto& comp = target.addComponent<T>();
        auto temp = source.getComponent<T>();
        comp = temp;
      }
    }
  }
  //----------------------------------------------------------------------------

  // Other helper functions.
  //----------------------------------------------------------------------------
  Entity
  createChildEntity(Entity entity, Shared<Scene> activeScene, const std::string &name = "New Entity")
  {
    if (!entity.hasComponent<ChildEntityComponent>())
    {
      // Add the child entity component.
      auto& children = entity.addComponent<ChildEntityComponent>();
      auto child = activeScene->createEntity(name);
      children.children.push_back(child);

      // Add the parent component to the child.
      child.addComponent<ParentEntityComponent>(entity);
      return child;
    }
    else
    {
      auto& children = entity.getComponent<ChildEntityComponent>();

      auto child = activeScene->createEntity(name);
      children.children.push_back(child);
      child.addComponent<ParentEntityComponent>(entity);
      return child;
    }
  }
  //----------------------------------------------------------------------------

  // Fixed selection bug for now -> removed the ability to deselect entities.
  // TODO: Re-add the ability to deselect entities.
  SceneGraphWindow::SceneGraphWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , materialEditor(parentLayer)
    , selectedString("")
    , fileTargets(FileLoadTargets::TargetNone)
    , saveTargets(FileSaveTargets::TargetNone)
    , dirWidgetShader("./assets/shaders/widgets/lightWidget.srshader")
    , widgetWidth(0.0f)
    , selectedSubmesh(nullptr)
  {
    auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
    this->dirBuffer = createShared<FrameBuffer>(512, 512);
    this->dirBuffer->attachTexture2D(cSpec);
    this->dirBuffer->attachRenderBuffer();
    this->dirBuffer->setClearColour(glm::vec4(0.0f));

    this->sphere.loadModel("./assets/.internal/sphere.fbx");
  }

  SceneGraphWindow::~SceneGraphWindow()
  { }

  void
  SceneGraphWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    ImGui::Begin("Scene Graph", &isOpen);

    static bool openPropWindow = true;
    ImGui::Checkbox("Show Component Properties", &openPropWindow);

    activeScene->sceneECS.each([&](auto entityID)
    {
      // Fetch the entity and its tag component.
      Entity current = Entity(entityID, activeScene.get());

      if (!current.hasComponent<ParentEntityComponent>())
        this->drawEntityNode(current, activeScene);
    });

    // Right-click on blank space to create a new entity.
		if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight, false))
		{
      if (ImGui::MenuItem("New Model"))
      {
				auto model = activeScene->createEntity("New Model");
        model.addComponent<TransformComponent>();
        model.addComponent<RenderableComponent>();
      }

      if (ImGui::BeginMenu("New Light"))
      {
        if (ImGui::MenuItem("Directional Light"))
        {
          auto light = activeScene->createEntity("New Directional Light");
          light.addComponent<DirectionalLightComponent>();
          light.addComponent<TransformComponent>();
        }

        if (ImGui::MenuItem("Point Light"))
        {
          auto light = activeScene->createEntity("New Point Light");
          light.addComponent<PointLightComponent>();
          light.addComponent<TransformComponent>();
        }

        if (ImGui::MenuItem("Spot Light"))
        {
          auto light = activeScene->createEntity("New Spot Light");
          light.addComponent<SpotLightComponent>();
          light.addComponent<TransformComponent>();
        }

        if (ImGui::MenuItem("Ambient Light"))
        {
          auto light = activeScene->createEntity("New Ambient Light");
          light.addComponent<AmbientComponent>();
        }

        ImGui::EndMenu();
      }

      if (ImGui::MenuItem("New Empty Entity"))
				activeScene->createEntity();

			ImGui::EndPopup();
		}

    ImGui::End();

    if (openPropWindow)
      this->drawPropsWindow(openPropWindow, activeScene);

    if (this->materialEditor.isOpen)
      this->materialEditor.onImGuiRender(this->materialEditor.isOpen, activeScene);
  }

  void
  SceneGraphWindow::onUpdate(float dt, Shared<Scene> activeScene)
  { }

  void
  SceneGraphWindow::onEvent(Event &event)
  {
    switch(event.getType())
    {
      // Swap the selected entity to something else.
      case EventType::EntitySwapEvent:
      {
        auto entSwapEvent = *(static_cast<EntitySwapEvent*>(&event));

        auto entityID = entSwapEvent.getStoredEntity();
        auto entityParentScene = entSwapEvent.getStoredScene();
        if (entityID < 0)
          this->selectedEntity = Entity();
        else
          this->selectedEntity = Entity((entt::entity) entityID, entityParentScene);

        this->selectedSubmesh = nullptr;
        this->materialEditor.isOpen = false;
        this->materialEditor.setSelectedMaterial("");

        break;
      }

      // Process a file loading event. Using enum barriers to prevent files from
      // being improperly loaded when this window didn't dispatch the event.
      // TODO: Bitmask instead of enums?
      case EventType::LoadFileEvent:
      {
        if (!this->selectedEntity)
          return;

        auto loadEvent = *(static_cast<LoadFileEvent*>(&event));
        auto& path = loadEvent.getAbsPath();
        auto& name = loadEvent.getFileName();

        switch (this->fileTargets)
        {
          case FileLoadTargets::TargetModel:
          {
            auto modelAssets = AssetManager<Model>::getManager();

            // If it already has a mesh component, remove it and add a new one.
            if (this->selectedEntity.hasComponent<RenderableComponent>())
              this->selectedEntity.removeComponent<RenderableComponent>();

            auto& renderable = this->selectedEntity.addComponent<RenderableComponent>(name);
            AsyncLoading::asyncLoadModel(path, name, this->selectedEntity, this->selectedEntity);

            this->fileTargets = FileLoadTargets::TargetNone;
            break;
          }

          case FileLoadTargets::TargetEnvironment:
          {
            auto environment = this->selectedEntity.getComponent<AmbientComponent>().ambient;
            auto state = Renderer3D::getState();

            environment->unloadEnvironment();
            environment->loadEquirectangularMap(path);
            environment->equiToCubeMap(true, state->skyboxWidth, state->skyboxWidth);
            environment->equiToCubeMap(true, state->skyboxWidth, state->skyboxWidth);
            environment->precomputeIrradiance(state->irradianceWidth, state->irradianceWidth, true);
            environment->precomputeSpecular(state->prefilterWidth, state->prefilterWidth, true);

            this->fileTargets = FileLoadTargets::TargetNone;
            break;
          }

          default: break;
        }
        break;
      }

      case EventType::SaveFileEvent:
      {
        if (!this->selectedEntity)
          return;

        auto saveEvent = *(static_cast<SaveFileEvent*>(&event));

        switch (this->saveTargets)
        {
          case FileSaveTargets::TargetPrefab:
          {
            auto& path = saveEvent.getAbsPath();
            auto& name = saveEvent.getFileName();
            auto fabName = name.substr(0, name.find_last_of('.'));

            this->selectedEntity.addComponent<PrefabComponent>(fabName, path);
            YAMLSerialization::serializePrefab(this->selectedEntity, path, fabName);
            break;
          }
        }

        this->saveTargets = FileSaveTargets::TargetNone;
        break;
      }
      default: break;
    }

    this->materialEditor.onEvent(event);
  }

  void
  SceneGraphWindow::drawEntityNode(Entity entity, Shared<Scene> activeScene)
  {
    auto& nameTag = entity.getComponent<NameComponent>().name;

    // Draw the entity + components as a treenode.
    ImGuiTreeNodeFlags flags = ((this->selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow
          | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (entity.hasComponent<PrefabComponent>())
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.54f, 0.11f, 0.0f, 1.0f));

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, nameTag.c_str());

    // Set the new selected entity.
    if (ImGui::IsItemClicked())
    {
      EventDispatcher* dispatcher = EventDispatcher::getInstance();
      dispatcher->queueEvent(new EntitySwapEvent(entity, activeScene.get()));

      this->selectedEntity = entity;
    }

    // Menu with entity properties. Allows the addition and deletion of
    // components, copying of the entity and deletion of the entity.
    if (ImGui::BeginPopupContextItem())
    {
      if (ImGui::BeginMenu("Attach Component"))
      {
        // Add various components.
        drawComponentAdd<TransformComponent>("Transform Component", entity);
        drawComponentAdd<RenderableComponent>("Renderable Component", entity);
        drawComponentAdd<CameraComponent>("Camera Component", entity);
        drawComponentAdd<DirectionalLightComponent>("Directional Light Component", entity);
        drawComponentAdd<PointLightComponent>("Point Light Component", entity);
        drawComponentAdd<SpotLightComponent>("Spot Light Component", entity);
        drawComponentAdd<AmbientComponent>("Ambient Light Component", entity);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Remove Component"))
      {
        // Remove various components.
        drawComponentRemove<TransformComponent>("Transform Component", entity);
        drawComponentRemove<RenderableComponent>("Renderable Component", entity);
        drawComponentRemove<CameraComponent>("Camera Component", entity);
        drawComponentRemove<DirectionalLightComponent>("Directional Light Component", entity);
        drawComponentRemove<PointLightComponent>("Point Light Component", entity);
        drawComponentRemove<SpotLightComponent>("Spot Light Component", entity);
        drawComponentRemove<AmbientComponent>("Ambient Light Component", entity);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Add Child Entity"))
      {
        if (ImGui::MenuItem("New Model"))
        {
  				auto model = createChildEntity(entity, activeScene, "New Model");
          model.addComponent<TransformComponent>();
          model.addComponent<RenderableComponent>();
        }

        if (ImGui::BeginMenu("New Light"))
        {
          if (ImGui::MenuItem("Directional Light"))
          {

            auto light = createChildEntity(entity, activeScene, "New Directional Light");
            light.addComponent<DirectionalLightComponent>();
            light.addComponent<TransformComponent>();
          }

          if (ImGui::MenuItem("Point Light"))
          {
            auto light = createChildEntity(entity, activeScene, "New Point Light");
            light.addComponent<PointLightComponent>();
            light.addComponent<TransformComponent>();
          }

          if (ImGui::MenuItem("Spot Light"))
          {
            auto light = createChildEntity(entity, activeScene, "New Spot Light");
            light.addComponent<SpotLightComponent>();
            light.addComponent<TransformComponent>();
          }

          if (ImGui::MenuItem("Ambient Light"))
          {
            auto light = createChildEntity(entity, activeScene, "New Ambient Light");
            light.addComponent<AmbientComponent>();
          }

          ImGui::EndMenu();
        }

        if (ImGui::MenuItem("New Empty Entity"))
  				createChildEntity(entity, activeScene);

        ImGui::EndMenu();
      }

      if (ImGui::MenuItem("Create Copy of Entity"))
      {
        auto newEntity = activeScene->createEntity();

        copyComponent<NameComponent>(entity, newEntity);
        copyComponent<TransformComponent>(entity, newEntity);
        copyComponent<RenderableComponent>(entity, newEntity);
        copyComponent<DirectionalLightComponent>(entity, newEntity);
        copyComponent<PointLightComponent>(entity, newEntity);
        copyComponent<SpotLightComponent>(entity, newEntity);
      }

      if (ImGui::MenuItem("Register as PreFab"))
      {
        EventDispatcher* dispatcher = EventDispatcher::getInstance();
        dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave, ".sfab"));

        this->saveTargets = FileSaveTargets::TargetPrefab;

        dispatcher->queueEvent(new EntitySwapEvent(entity, activeScene.get()));

        this->selectedEntity = entity;
      }

      // Check to see if we should delete the entity.
      if (ImGui::MenuItem("Delete Entity"))
      {
        EventDispatcher* dispatcher = EventDispatcher::getInstance();
        dispatcher->queueEvent(new EntitySwapEvent(-1, activeScene.get()));
        dispatcher->queueEvent(new EntityDeleteEvent(entity, activeScene.get()));
      }

      ImGui::EndPopup();
    }

    // Open the list of attached components.
    if (opened)
    {
      this->drawComponentNodes(entity, activeScene);
      ImGui::TreePop();
    }

    if (entity.hasComponent<PrefabComponent>())
      ImGui::PopStyleColor();
  }

  // Function for drawing sub-entities and components of an entity.
  void
  SceneGraphWindow::drawComponentNodes(Entity entity, Shared<Scene> activeScene)
  {
    if (!entity)
      return;

    ImGuiTreeNodeFlags leafFlag = ImGuiTreeNodeFlags_Leaf
                                | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    // Display the child entities
    if (entity.hasComponent<ChildEntityComponent>())
    {
      auto& children = entity.getComponent<ChildEntityComponent>();
      for (auto& child : children.children)
        drawEntityNode(child, activeScene);
    }

    // Display components here.
    if (entity.hasComponent<TransformComponent>())
    {
      ImGui::TreeNodeEx("Transform Componenet", leafFlag);
    }
    if (entity.hasComponent<RenderableComponent>())
    {
      ImGui::TreeNodeEx("Renderable Componenet", leafFlag);
    }
    if (entity.hasComponent<CameraComponent>())
    {
      ImGui::TreeNodeEx("Camera Componenet", leafFlag);
    }
    if (entity.hasComponent<DirectionalLightComponent>())
    {
      ImGui::TreeNodeEx("Directional Light Componenet", leafFlag);
    }
    if (entity.hasComponent<PointLightComponent>())
    {
      ImGui::TreeNodeEx("Point Light Componenet", leafFlag);
    }
    if (entity.hasComponent<SpotLightComponent>())
    {
      ImGui::TreeNodeEx("Spot Light Componenet", leafFlag);
    }
    if (entity.hasComponent<AmbientComponent>())
    {
      ImGui::TreeNodeEx("Ambient Light Componenet", leafFlag);
    }
  }

  // The property panel for an entity.
  void
  SceneGraphWindow::drawPropsWindow(bool &isOpen, Shared<Scene> activeScene)
  {
    ImGui::Begin("Components", &isOpen);
    if (this->selectedEntity)
    {
      auto& name = this->selectedEntity.getComponent<NameComponent>().name;
      auto& description = this->selectedEntity.getComponent<NameComponent>().description;

      char nameBuffer[256];
      memset(nameBuffer, 0, sizeof(nameBuffer));
      std::strncpy(nameBuffer, name.c_str(), sizeof(nameBuffer));

      char descBuffer[256];
      memset(descBuffer, 0, sizeof(descBuffer));
      std::strncpy(descBuffer, description.c_str(), sizeof(descBuffer));

      ImGui::Text("Name:");
      if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer)))
        name = std::string(nameBuffer);

      ImGui::Text("Description:");
      if (ImGui::InputText("##desc", descBuffer, sizeof(descBuffer)))
        description = std::string(descBuffer);

      if (this->selectedEntity.hasComponent<PrefabComponent>())
      {
        auto& pfComponent = this->selectedEntity.getComponent<PrefabComponent>();

        ImGui::Text(" ");
        ImGui::Text("Prefab Settings");
        ImGui::Separator();
        ImGui::Checkbox("Synch Prefab", &pfComponent.synch);

        auto tempID = pfComponent.prefabID;
        auto tempPath = pfComponent.prefabPath;

        ImGui::Text((std::string("Prefab ID: ") + tempID).c_str());
        ImGui::Text((std::string("Prefab path: ") + tempPath).c_str());
        if (pfComponent.synch)
        {
          if (ImGui::Button("Push Prefab Settings"))
          {
            YAMLSerialization::serializePrefab(this->selectedEntity, pfComponent.prefabPath, pfComponent.prefabID);

            auto autoPrefabs = activeScene->sceneECS.view<PrefabComponent>();
            for (auto entity : autoPrefabs)
            {
              auto prefab = autoPrefabs.get<PrefabComponent>(entity);
              if (prefab.synch && prefab.prefabID == tempID)
              {
                EventDispatcher* dispatcher = EventDispatcher::getInstance();
                dispatcher->queueEvent(new EntitySwapEvent(-1, activeScene.get()));
                dispatcher->queueEvent(new EntityDeleteEvent((int) entity, activeScene.get()));

                YAMLSerialization::deserializePrefab(activeScene, tempPath);
              }
            }
          }
        }
      }

      drawComponentProperties<TransformComponent>("Transform Component",
        this->selectedEntity, [](auto& component)
      {
        Styles::drawVec3Controls("Translation", glm::vec3(0.0f), component.translation);
        glm::vec3 tEulerRotation = glm::degrees(component.rotation);
        Styles::drawVec3Controls("Rotation", glm::vec3(0.0f), tEulerRotation);
        component.rotation = glm::radians(tEulerRotation);
        Styles::drawVec3Controls("Scale", glm::vec3(1.0f), component.scale);
      });

      drawComponentProperties<RenderableComponent>("Renderable Component",
        this->selectedEntity, [this](auto& component)
      {
        Model* componentModel = component;
        char nameBuffer[256];
        memset(nameBuffer, 0, sizeof(nameBuffer));
        if (componentModel)
          std::strncpy(nameBuffer, componentModel->getFilepath().c_str(), sizeof(nameBuffer));

        ImGui::Text("Mesh Information");
        ImGui::Separator();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN))
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                       ".obj,.FBX,.fbx"));

          this->fileTargets = FileLoadTargets::TargetModel;
        }

        ImGui::SameLine();
        ImGui::InputText("##modelPath", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_ReadOnly);
        this->loadDNDAsset();
        ImGui::Button("Open Model Viewer");

        ImGui::Text("");
        ImGui::Separator();
        ImGui::Text("Materials");
        if (this->selectedSubmesh)
        {
          if (ImGui::BeginCombo("##sceneGraphSelectedSubmesh", this->selectedSubmesh->getName().c_str()))
          {
            for (auto& submesh : componentModel->getSubmeshes())
            {
              bool isSelected = (&submesh == this->selectedSubmesh);

              if (ImGui::Selectable(submesh.getName().c_str(), isSelected))
                this->selectedSubmesh = (&submesh);

              if (isSelected)
                ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
          }

          std::string submeshName = this->selectedSubmesh->getName();
          auto materialHandle = component.materials.getMaterialHandle(submeshName);
          auto material = component.materials.getMaterial(this->selectedSubmesh->getName());

          if (material)
          {
            ImGui::PushID("MaterialPreview");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D("albedoMap")->getID(),
                                   ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {

            }
            this->loadDNDAsset(this->selectedSubmesh->getName());
            ImGui::PopID();

            if (ImGui::Button("Edit Material"))
            {
              this->materialEditor.isOpen = true;
              this->materialEditor.setSelectedMaterial(materialHandle);
            }
          }
        }
        else
        {
          if (ImGui::BeginCombo("##sceneGraphSelectedSubmesh", ""))
          {
            for (auto& submesh : componentModel->getSubmeshes())
            {
              bool isSelected = (&submesh == this->selectedSubmesh);

              if (ImGui::Selectable(submesh.getName().c_str(), isSelected))
                this->selectedSubmesh = (&submesh);

              if (isSelected)
                ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
          }
        }

        if (componentModel)
        {
          if (componentModel->getAnimations().size() > 0)
          {
            ImGui::Text("");
            ImGui::Separator();
            ImGui::Text("Animations");
            auto storedAnimation = component.animator.getStoredAnimation();
            if (storedAnimation)
            {
              if (ImGui::BeginCombo("##animator", storedAnimation->getName().c_str()))
              {
                for (auto& animation : componentModel->getAnimations())
                {
                  bool isSelected = (&animation == storedAnimation);
          
                  if (ImGui::Selectable(animation.getName().c_str(), isSelected))
                    component.animator.setAnimation(&animation, component.meshName);
          
                  if (isSelected)
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
              }
          
              ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
              if (component.animator.isPaused())
              {
                if (ImGui::Button(ICON_FA_PLAY))
                  component.animator.startAnimation();
              }
              else
              {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                ImGui::Button(ICON_FA_PLAY);
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
              }
          
              ImGui::SameLine();
              if (!component.animator.isPaused())
              {
                if (ImGui::Button(ICON_FA_PAUSE))
                  component.animator.pauseAnimation();
              }
              else
              {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                ImGui::Button(ICON_FA_PAUSE);
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
              }
          
              ImGui::SameLine();
              if (component.animator.isAnimating())
              {
                if (ImGui::Button(ICON_FA_STOP))
                  component.animator.stopAnimation();
              }
              else
              {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                ImGui::Button(ICON_FA_STOP);
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
              }
              ImGui::PopStyleVar();
          
              ImGui::SliderFloat("##AnimationTime", &component.animator.getAnimationTime(), 0.0f, storedAnimation->getDuration());
            }
            else
            {
              if (ImGui::BeginCombo("##animator", ""))
              {
                for (auto& animation : componentModel->getAnimations())
                {
                  bool isSelected = (&animation == storedAnimation);
          
                  if (ImGui::Selectable(animation.getName().c_str(), isSelected))
                    component.animator.setAnimation(&animation, component.meshName);
          
                  if (isSelected)
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
              }
            }
          }
        }
      });

      drawComponentProperties<CameraComponent>("Camera Component",
        this->selectedEntity, [this, activeScene](auto& component)
      {
        auto& camera = component.entCamera;

        float degFOV = glm::degrees(camera.fov);

        ImGui::Checkbox("Primary Camera", &component.isPrimary);
        Styles::drawFloatControl("Near", 0.1f, camera.near, 0.0f, 0.1f, 0.1f, 100.0f);
        Styles::drawFloatControl("Far", 30.0f, camera.far, 0.0f, 0.1f, 30.0f, 500.0f);
        Styles::drawFloatControl("FOV", 45.0f, degFOV, 0.0f, 0.1f, 30.0f, 180.0f);

        camera.fov = glm::radians(degFOV);
      });

      drawComponentProperties<DirectionalLightComponent>("Directional Light Component",
        this->selectedEntity, [this, activeScene](auto& component)
      {
        ImGui::PushID("DirectionalLight");

        bool isPrimaryLight = component.light.primaryLight;
        ImGui::Checkbox("Primary Light", &component.light.primaryLight);
        if (component.light.primaryLight && !isPrimaryLight)
        {
          auto dirLight = activeScene->sceneECS.view<DirectionalLightComponent>();

          for (auto entity : dirLight)
          {
            auto& directional = dirLight.get<DirectionalLightComponent>(entity);
            if (this->selectedEntity != entity && directional.light.primaryLight)
              directional.light.primaryLight;
          }
        }

        ImGui::Checkbox("Cast Shadows", &component.light.castShadows);
        ImGui::ColorEdit3("Colour", &component.light.colour.r);
        Styles::drawFloatControl("Intensity", 0.0f, component.light.intensity,
                                 0.0f, 0.01f, 0.0f, 100.0f);
        ImGui::PopID();

        this->drawDirectionalWidget();
      });

      drawComponentProperties<PointLightComponent>("Point Light Component",
        this->selectedEntity, [this](auto& component)
      {
        ImGui::PushID("PointLight");
        ImGui::Checkbox("Cast Shadows", &component.light.castShadows);
        ImGui::ColorEdit3("Colour", &component.light.colour.r);
        Styles::drawFloatControl("Radius", 0.0f, component.light.radius, 0.0f, 0.1f, 0.0f, 100.0f);
        Styles::drawFloatControl("Falloff", 0.0f, component.light.falloff, 0.0f, 0.01f, 0.0f, 1.0f);
        Styles::drawFloatControl("Intensity", 0.0f, component.light.intensity,
                                 0.0f, 0.01f, 0.0f, 100.0f);
        ImGui::PopID();
      });

      drawComponentProperties<SpotLightComponent>("Spot Light Component",
        this->selectedEntity, [this](auto& component)
      {
        ImGui::PushID("SpotLight");
        ImGui::Checkbox("Cast Shadows", &component.light.castShadows);
        ImGui::ColorEdit3("Colour", &component.light.colour.r);
        Styles::drawFloatControl("Radius", 0.0f, component.light.radius, 0.0f, 0.1f, 0.0f, 100.0f);
        Styles::drawFloatControl("Intensity", 0.0f, component.light.intensity,
                                 0.0f, 0.1f, 0.0f, 100.0f);
        float innerAngle = glm::degrees(std::acos(component.light.innerCutoff));
        Styles::drawFloatControl("Inner Cutoff", 45.0f, innerAngle,
                                 0.0f, 0.1f, 0.0f, 360.0f);
        component.light.innerCutoff = std::cos(glm::radians(innerAngle));
        float outerAngle = glm::degrees(std::acos(component.light.outerCutoff));
        Styles::drawFloatControl("Outer Cutoff", 90.0f, outerAngle,
                                 0.0f, 0.1f, 0.0f, 360.0f);
        component.light.outerCutoff = std::cos(glm::radians(outerAngle));
        ImGui::PopID();
      });

      drawComponentProperties<AmbientComponent>("Ambient Light Component",
        this->selectedEntity, [this](auto& component)
      {
        if (ImGui::Button("Load New Environment"))
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen, ".hdr"));

          this->fileTargets = FileLoadTargets::TargetEnvironment;
        }

        EnvironmentMap* env = component.ambient;

        auto drawingType = env->getDrawingType();
        static uint mapTypes[] = { 0, 1, 2, 3 };
        static uint skyTypes[] = { 0, 1 };

        if (ImGui::BeginCombo("##ambientType", EnvironmentMap::mapEnumToString(drawingType).c_str()))
        {
          for (uint i = 0; i < IM_ARRAYSIZE(mapTypes); i++)
          {
            bool isSelected = (i == static_cast<uint>(drawingType));
            auto mapTypeString = EnvironmentMap::mapEnumToString(static_cast<MapType>(mapTypes[i]));

            if (ImGui::Selectable(mapTypeString.c_str(), isSelected))
            {
              auto drawingType = static_cast<MapType>(i);
              env->setDrawingType(drawingType);
              if (drawingType == MapType::DynamicSky)
                env->setDynamicSkyIBL();
              else
                env->setStaticIBL();
            }
            if (isSelected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        ImGui::Separator();

        if (env->getDrawingType() == MapType::DynamicSky)
        {
          auto dynamicSkyType = env->getDynamicSkyType();

          if (this->selectedEntity.hasComponent<TransformComponent>())
          {
            ImGui::Checkbox("Aniamte Dynamic Sky", &component.animate);
            if (component.animate)
            {
              ImGui::DragFloat("Animation Speed", &component.animationSpeed, 0.001f);
            }
          }

          ImGui::Text("");
          if (ImGui::BeginCombo("##dynamicSkyType", EnvironmentMap::skyEnumToString(dynamicSkyType).c_str()))
          {
            for (uint i = 0; i < IM_ARRAYSIZE(skyTypes); i++)
            {
              bool isSelected = (i == static_cast<uint>(dynamicSkyType));
              auto skyTypeString = EnvironmentMap::skyEnumToString(static_cast<DynamicSkyType>(skyTypes[i]));

              if (ImGui::Selectable(skyTypeString.c_str(), isSelected))
                env->setDynamicSkyType(static_cast<DynamicSkyType>(i));
              if (isSelected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }

          if (env->getDynamicSkyType() == DynamicSkyType::Preetham)
          {
            auto preethamParams = env->getSkyParams<PreethamSkyParams>(DynamicSkyType::Preetham);

            ImGui::Indent();
            if (ImGui::CollapsingHeader("Common Sky Parameters##PreethamAtmo"))
            {
              Styles::drawFloatControl("Sun Size", 1.5f, preethamParams.sunSize, 0.0f, 0.1f, 0.0f, 10.0f);
              Styles::drawFloatControl("Sun Intensity", 1.0f, preethamParams.sunIntensity, 0.0f, 0.1f, 0.0f, 10.0f);
              Styles::drawFloatControl("Sky Intensity", 1.0f, preethamParams.skyIntensity, 0.0f, 0.1f, 0.0f, 10.0f);
            }

            if (ImGui::CollapsingHeader("Preetham Parameters##PreethamAtmo"))
            {
              Styles::drawFloatControl("Turbidity", 2.0f, preethamParams.turbidity, 0.0f, 0.1f, 2.0f, 10.0f);
            }
            ImGui::Unindent();

            env->setSkyModelParams<PreethamSkyParams>(preethamParams);
          }

          if (env->getDynamicSkyType() == DynamicSkyType::Hillaire)
          {
            auto hillaireParams = env->getSkyParams<HillaireSkyParams>(DynamicSkyType::Hillaire);

            ImGui::Indent();
            if (ImGui::CollapsingHeader("Common Sky Parameters##PreethamAtmo"))
            {
              Styles::drawFloatControl("Sun Size", 1.5f, hillaireParams.sunSize, 0.0f, 0.1f, 0.0f, 10.0f);
              Styles::drawFloatControl("Sun Intensity", 1.0f, hillaireParams.sunIntensity, 0.0f, 0.1f, 0.0f, 10.0f);
              Styles::drawFloatControl("Sky Intensity", 1.0f, hillaireParams.skyIntensity, 0.0f, 0.1f, 0.0f, 10.0f);
            }

            if (ImGui::CollapsingHeader("Scattering Parameters##UE4Atmo"))
            {
              Styles::drawVec3Controls("Rayleigh Scattering", glm::vec3(5.802f, 13.558f, 33.1f),
                                       hillaireParams.rayleighScatteringBase, 0.0f, 0.1f,
                                       0.0f, 100.0f);
              Styles::drawFloatControl("Rayleigh Absorption", 0.0f,
                                       hillaireParams.rayleighAbsorptionBase, 0.0f, 0.1f,
                                       0.0f, 100.0f);
              Styles::drawFloatControl("Mie Scattering", 3.996f, hillaireParams.mieScatteringBase,
                                       0.0f, 0.1f, 0.0f, 100.0f);
              Styles::drawFloatControl("Mie Absorption", 4.4f, hillaireParams.mieAbsorptionBase,
                                       0.0f, 0.1f, 0.0f, 100.0f);
              Styles::drawVec3Controls("Ozone Absorption", glm::vec3(0.650f, 1.881f, 0.085f),
                                       hillaireParams.ozoneAbsorptionBase, 0.0f, 0.1f,
                                       0.0f, 100.0f);
            }

            if (ImGui::CollapsingHeader("Planetary Parameters##UE4Atmo"))
            {
              float planetRadiusKM = hillaireParams.planetRadius * 1000.0f;
              float atmosphereRadiusKM = hillaireParams.atmosphereRadius * 1000.0f;
              glm::vec3 viewPosKM = hillaireParams.viewPos * 1000.0f;
              Styles::drawFloatControl("Planet Radius (Km)",
                                       6360.0f, planetRadiusKM,
                                       0.0f, 1.0f, 0.0f, atmosphereRadiusKM);
              Styles::drawFloatControl("Atmosphere Radius (Km)",
                                       6460.0f, atmosphereRadiusKM,
                                       0.0f, 1.0f, planetRadiusKM, 10000.0f);
              Styles::drawFloatControl("View Height", 6360.0f + 0.2f,
                                       viewPosKM.y, 0.0f, 0.1f,
                                       planetRadiusKM,
                                       atmosphereRadiusKM);
              hillaireParams.planetRadius = planetRadiusKM / 1000.0f;
              hillaireParams.atmosphereRadius = atmosphereRadiusKM / 1000.0f;
              hillaireParams.viewPos = viewPosKM / 1000.0f;
            }
            ImGui::Unindent();

            env->setSkyModelParams<HillaireSkyParams>(hillaireParams);
          }
        }

        if (env->getDrawingType() == MapType::Prefilter)
          ImGui::SliderFloat("Roughness", &env->getRoughness(), 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("");
        Styles::drawFloatControl("Ambient Intensity", 0.5f, env->getIntensity(), 0.0f, 0.1f, 0.0f, 100.0f);
      });
    }

    ImGui::End();
  }

  // Draw a widget to make controlling directional lights more intuitive.
  void
  SceneGraphWindow::drawDirectionalWidget()
  {
    if (this->selectedEntity.hasComponent<TransformComponent>() &&
        this->selectedEntity.hasComponent<DirectionalLightComponent>())
    {
      auto model = glm::mat4(1.0);
      auto viewPos = glm::vec3(2.0f);
      auto viewDir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - viewPos);
      auto view = glm::lookAt(viewPos, viewPos + viewDir, glm::vec3(0.0f, 1.0f, 0.0f));
      auto projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
      auto mVP = projection * view * model;

      auto& transform = this->selectedEntity.getComponent<TransformComponent>();
      auto& light = this->selectedEntity.getComponent<DirectionalLightComponent>();

      auto lightDir = -1.0f * glm::vec3(glm::toMat4(glm::quat(transform.rotation))
                            * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));

      this->dirBuffer->clear();
      this->dirBuffer->bind();
      this->dirBuffer->setViewport();

      this->dirWidgetShader.addUniformMatrix("mVP", mVP, false);
      this->dirWidgetShader.addUniformMatrix("normalMat", glm::transpose(glm::inverse(glm::mat3(model))), false);
      this->dirWidgetShader.addUniformMatrix("model", model, false);
      this->dirWidgetShader.addUniformVector("lDirection", lightDir);

      for (auto& submesh : this->sphere.getSubmeshes())
      {
        if (submesh.hasVAO())
          Renderer3D::draw(submesh.getVAO(), &this->dirWidgetShader);
        else
        {
          submesh.generateVAO();
          if (submesh.hasVAO())
            Renderer3D::draw(submesh.getVAO(), &this->dirWidgetShader);
        }
      }

      this->dirBuffer->unbind();

      this->widgetWidth = ImGui::GetWindowSize().x * 0.75f;
      ImGui::BeginChild("LightDirection", ImVec2(this->widgetWidth, this->widgetWidth));
      ImGui::Image((ImTextureID) (unsigned long) this->dirBuffer->getAttachID(FBOTargetParam::Colour0),
                   ImVec2(this->widgetWidth, this->widgetWidth), ImVec2(0, 1), ImVec2(1, 0));

      // ImGuizmo boilerplate. Prepare the drawing context and set the window to
      // draw the gizmos to.
      ImGuizmo::SetOrthographic(false);
      ImGuizmo::SetDrawlist();

      auto windowMin = ImGui::GetWindowContentRegionMin();
      auto windowMax = ImGui::GetWindowContentRegionMax();
      auto windowOffset = ImGui::GetWindowPos();
      ImVec2 bounds[2];
      bounds[0] = ImVec2(windowMin.x + windowOffset.x,
                         windowMin.y + windowOffset.y);
      bounds[1] = ImVec2(windowMax.x + windowOffset.x,
                         windowMax.y + windowOffset.y);

      ImGuizmo::SetRect(bounds[0].x - 100.0f, bounds[0].y - 100.0f,
                        (bounds[1].x - bounds[0].x) + 200.0f,
                        (bounds[1].y - bounds[0].y) + 200.0f);

      glm::mat4 transformMatrix = transform;

      // Manipulate the matrix. TODO: Add snapping.
      ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                           ImGuizmo::ROTATE, ImGuizmo::WORLD,
                           glm::value_ptr(transformMatrix), nullptr, nullptr);

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

      ImGui::EndChild();
    }
  }

  void
  SceneGraphWindow::loadDNDAsset()
  {
    if (!this->selectedEntity)
      return;

    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
      {
        auto modelAssets = AssetManager<Model>::getManager();

        std::string filepath = (char*) payload->Data;
        std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
        std::string filetype = filename.substr(filename.find_last_of('.'));

        // Attach a mesh component.
        if (filetype == ".obj" || filetype == ".FBX" || filetype == ".fbx"
            || filetype == ".blend" || filetype == ".gltf" || filetype == ".glb"
            || filetype == ".dae")
        {
          // If it already has a mesh component, remove it and add a new one.
          // Otherwise just add a component.
          if (this->selectedEntity.hasComponent<RenderableComponent>())
            this->selectedEntity.removeComponent<RenderableComponent>();

          auto& renderable = this->selectedEntity.addComponent<RenderableComponent>(filename);
          AsyncLoading::asyncLoadModel(filepath, filename, this->selectedEntity, this->selectedEntity);
        }
      }

      ImGui::EndDragDropTarget();
    }
  }

  void
  SceneGraphWindow::loadDNDAsset(const std::string &submeshName)
  {
    if (!this->selectedEntity)
      return;

    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
      {
        auto materialAssets = AssetManager<Material>::getManager();

        std::string filepath = (char*) payload->Data;
        std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
        std::string filetype = filename.substr(filename.find_last_of('.'));

        if (filetype == ".smtl")
        {
          AssetHandle handle;
          if (YAMLSerialization::deserializeMaterial(filepath, handle))
          {
            materialAssets->getAsset(handle)->getFilepath() = filepath;

            auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();
            rComponent.materials.swapMaterial(submeshName, handle);
          }
        }
      }

      ImGui::EndDragDropTarget();
    }
  }
}
