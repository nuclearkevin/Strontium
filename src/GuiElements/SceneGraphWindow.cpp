#include "GuiElements/SceneGraphWindow.h"

// STL includes.
#include <cstring>

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"
#include "Scenes/Components.h"
#include "Serialization/YamlSerialization.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
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

  // Helper function to create a child entity.
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

  // Helper function to recursively delete child entities.
  void
  SceneGraphWindow::recursiveChildDelete(Entity parent, Shared<Scene> activeScene)
  {
    if (parent.hasComponent<ChildEntityComponent>())
    {
      auto& children = parent.getComponent<ChildEntityComponent>().children;
      for (auto& child : children)
        recursiveChildDelete(child, activeScene);
    }

    if (parent == this->selectedEntity)
      this->deleteSelected = true;
    else
      activeScene->deleteEntity(parent);
  }

  // Fixed selection bug for now -> removed the ability to deselect entities.
  // TODO: Readd the ability to deselect entities.
  SceneGraphWindow::SceneGraphWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , selectedString("")
    , fileTargets(FileLoadTargets::TargetNone)
    , saveTargets(FileSaveTargets::TargetNone)
    , deleteSelected(false)
  { }

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
		if (ImGui::BeginPopupContextWindow(0, 1, false))
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
        }

        if (ImGui::MenuItem("Point Light"))
        {
          auto light = activeScene->createEntity("New Point Light");
          light.addComponent<PointLightComponent>();
        }

        if (ImGui::MenuItem("Spot Light"))
        {
          auto light = activeScene->createEntity("New Spot Light");
          light.addComponent<SpotLightComponent>();
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
  }

  void
  SceneGraphWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

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
            Model::asyncLoadModel(path, name, &renderable.materials);

            this->fileTargets = FileLoadTargets::TargetNone;
            break;
          }

          case FileLoadTargets::TargetEnvironment:
          {
            auto& ambient = this->selectedEntity.getComponent<AmbientComponent>();

            if (ambient.ambient->hasEqrMap())
            {
              ambient.ambient->unloadEnvironment();
              ambient.ambient->loadEquirectangularMap(path);
              ambient.ambient->equiToCubeMap(true, 2048, 2048);
              ambient.ambient->precomputeIrradiance(512, 512, true);
              ambient.ambient->precomputeSpecular(2048, 2048, true);
            }
            else
            {
              ambient.ambient->loadEquirectangularMap(path);
              ambient.ambient->equiToCubeMap(true, 2048, 2048);
              ambient.ambient->precomputeIrradiance(512, 512, true);
              ambient.ambient->precomputeSpecular(2048, 2048, true);
            }

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
  }

  void
  SceneGraphWindow::drawEntityNode(Entity entity, Shared<Scene> activeScene)
  {
    auto& nameTag = entity.getComponent<NameComponent>().name;

    // Draw the entity + components as a treenode.
    ImGuiTreeNodeFlags flags = ((this->selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow
          | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, nameTag.c_str());

    // Drag and drop targets!
    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
      {
        EventDispatcher* dispatcher = EventDispatcher::getInstance();
        dispatcher->queueEvent(new EntitySwapEvent(entity, activeScene.get()));

        this->selectedEntity = entity;
        this->loadDNDAsset((char*) payload->Data);
      }

      ImGui::EndDragDropTarget();
    }

    // Set the new selected entity.
    if (ImGui::IsItemClicked())
    {
      EventDispatcher* dispatcher = EventDispatcher::getInstance();
      dispatcher->queueEvent(new EntitySwapEvent(entity, activeScene.get()));

      this->selectedEntity = entity;
    }

    // Menu with entity properties. Allows the addition and deletion of
    // components, copying of the entity and deletion of the entity.
    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
      if (ImGui::BeginMenu("Attach Component"))
      {
        // Add various components.
        drawComponentAdd<TransformComponent>("Transform Component", entity);
        drawComponentAdd<RenderableComponent>("Renderable Component", entity);
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
          }

          if (ImGui::MenuItem("Point Light"))
          {
            auto light = createChildEntity(entity, activeScene, "New Point Light");
            light.addComponent<PointLightComponent>();
          }

          if (ImGui::MenuItem("Spot Light"))
          {
            auto light = createChildEntity(entity, activeScene, "New Spot Light");
            light.addComponent<SpotLightComponent>();
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
                this->recursiveChildDelete(Entity(entity, activeScene.get()), activeScene);
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
        static bool showMeshWindow = false;

        ImGui::Text((std::string("Mesh: ") + component.meshName).c_str());
        if (ImGui::Button("Select New Mesh"))
          showMeshWindow = true;

        if (showMeshWindow)
          this->drawMeshWindow(showMeshWindow);
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
                                 0.0f, 0.1f, 0.0f, 100.0f);
        Styles::drawVec3Controls("Direction", glm::vec3(0.0f), component.light.direction,
                                 0.0f, 0.01f, -1.0f, 1.0f);
        ImGui::PopID();
      });

      drawComponentProperties<PointLightComponent>("Point Light Component",
        this->selectedEntity, [this](auto& component)
      {
        ImGui::PushID("PointLight");
        ImGui::Checkbox("Cast Shadows", &component.light.castShadows);
        ImGui::ColorEdit3("Colour", &component.light.colour.r);
        Styles::drawFloatControl("Radius", 0.0f, component.light.radius, 0.0f, 0.1f, 0.0f, 100.0f);
        Styles::drawFloatControl("Intensity", 0.0f, component.light.intensity,
                                 0.0f, 0.1f, 0.0f, 10.0f);
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
        GLfloat innerAngle = glm::degrees(std::acos(component.light.innerCutoff));
        Styles::drawFloatControl("Inner Cutoff", 45.0f, innerAngle,
                                 0.0f, 0.1f, 0.0f, 360.0f);
        component.light.innerCutoff = std::cos(glm::radians(innerAngle));
        GLfloat outerAngle = glm::degrees(std::acos(component.light.outerCutoff));
        Styles::drawFloatControl("Outer Cutoff", 90.0f, outerAngle,
                                 0.0f, 0.1f, 0.0f, 360.0f);
        component.light.outerCutoff = std::cos(glm::radians(outerAngle));
        ImGui::PopID();
      });

      drawComponentProperties<AmbientComponent>("Ambient Light Component",
        this->selectedEntity, [this](auto& component)
      {
        bool attachEnvi = false;
        auto storage = Renderer3D::getStorage();

        static bool drawingMips = false;

        if (ImGui::Button("Load New Environment"))
          attachEnvi = true;

        if (component.ambient->hasEqrMap())
        {
          ImGui::Checkbox("Draw Blurred", &drawingMips);
          if (drawingMips)
          {
            ImGui::SliderFloat("Roughness", &component.ambient->getRoughness(), 0.0f, 1.0f);
            component.ambient->setDrawingType(MapType::Prefilter);
          }
          else
            component.ambient->setDrawingType(MapType::Skybox);

          Styles::drawFloatControl("Intensity", 1.0f, component.ambient->getIntensity(), 0.0f, 0.1f, 0.0f, 100.0f);
        }

        if (attachEnvi)
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen, ".hdr"));

          this->fileTargets = FileLoadTargets::TargetEnvironment;
        }
      });
    }

    ImGui::End();
  }

  void
  SceneGraphWindow::loadDNDAsset(const std::string &filepath)
  {
    if (!this->selectedEntity)
      return;

    auto modelAssets = AssetManager<Model>::getManager();

    std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
    std::string filetype = filename.substr(filename.find_last_of('.'));

    // Attach a mesh component.
    if (filetype == ".obj" || filetype == ".FBX" || filetype == ".fbx")
    {
      // If it already has a mesh component, remove it and add a new one.
      // Otherwise just add a component.
      if (this->selectedEntity.hasComponent<RenderableComponent>())
        this->selectedEntity.removeComponent<RenderableComponent>();

      auto& renderable = this->selectedEntity.addComponent<RenderableComponent>(filename);
      Model::asyncLoadModel(filepath, filename, &renderable.materials);
    }
  }

  // Draw the mesh selection window.
  void
  SceneGraphWindow::drawMeshWindow(bool &isOpen)
  {
    // Get the asset caches.
    auto modelAssets = AssetManager<Model>::getManager();
    auto shaderCache = AssetManager<Shader>::getManager();

    ImGui::Begin("Mesh Assets", &isOpen);

    // Button to load in a new mesh asset.
    if (ImGui::Button("Load New Mesh"))
    {
      EventDispatcher* dispatcher = EventDispatcher::getInstance();
      dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                   ".obj,.FBX,.fbx"));

      this->fileTargets = FileLoadTargets::TargetModel;
      isOpen = false;
    }

    // Loop over all the loaded models and display each for selection.
    for (auto& name : modelAssets->getStorage())
    {
      ImGuiTreeNodeFlags flags = ((this->selectedString == name) ? ImGuiTreeNodeFlags_Selected : 0);
      flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_Bullet;

      bool opened = ImGui::TreeNodeEx((void*) (sizeof(char) * name.size()), flags, name.c_str());

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        if (this->selectedEntity.hasComponent<RenderableComponent>())
          this->selectedEntity.removeComponent<RenderableComponent>();

        auto& renderable = this->selectedEntity.addComponent<RenderableComponent>(name);

        isOpen = false;
        this->selectedString = "";
      }
    }
    ImGui::End();

    if (!isOpen)
      this->selectedString = "";
  }
}
