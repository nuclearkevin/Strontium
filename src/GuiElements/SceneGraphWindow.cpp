#include "GuiElements/SceneGraphWindow.h"

// STL includes.
#include <cstring>

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"

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

  // Fixed selection bug for now -> removed the ability to deselect entities.
  // TODO: Readd the ability to deselect entities.
  SceneGraphWindow::SceneGraphWindow()
    : GuiWindow()
    , selectedString("")
    , fileTargets(FileLoadTargets::TargetNone)
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
      this->drawEntityNode(current, activeScene);
    });

    // Right-click on blank space to create a new entity.
		if (ImGui::BeginPopupContextWindow(0, 1, false))
		{
			if (ImGui::MenuItem("Create New Empty Entity"))
				activeScene->createEntity();

      // Create a debug bunny entity.
      if (ImGui::MenuItem("Create New Model"))
      {
        auto shaderCache = AssetManager<Shader>::getManager();
        auto modelAssets = AssetManager<Model>::getManager();

				auto model = activeScene->createEntity("New Model");
        model.addComponent<TransformComponent>();
        model.addComponent<RenderableComponent>();
      }

			ImGui::EndPopup();
		}
    ImGui::End();

    if (openPropWindow)
      this->drawPropsWindow(openPropWindow);
  }

  void
  SceneGraphWindow::onUpdate(float dt)
  {

  }

  void
  SceneGraphWindow::onEvent(Event &event)
  {
    if (!this->selectedEntity)
      return;

    switch(event.getType())
    {
      // Process a file loading event. Using enum barriers to prevent files from
      // being improperly loaded when this window didn't dispatch the event.
      case EventType::LoadFileEvent:
      {
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
            {
              this->selectedEntity.removeComponent<RenderableComponent>();
              auto& renderable = this->selectedEntity.addComponent<RenderableComponent>
                (modelAssets->loadAssetFile(path, name), name);
            }

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

    // Set the new selected entity.
    if (ImGui::IsItemClicked())
      this->selectedEntity = entity;

    // Menu with entity properties. Allows the addition and deletion of
    // components, copying of the entity and deletion of the entity.
    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
      if (ImGui::BeginMenu("Attach Component"))
      {
        this->selectedEntity = entity;

        // Add various components.
        drawComponentAdd<TransformComponent>("Transform Component", this->selectedEntity);
        drawComponentAdd<RenderableComponent>("Renderable Component", this->selectedEntity);
        drawComponentAdd<DirectionalLightComponent>("Directional Light Component", this->selectedEntity);
        drawComponentAdd<PointLightComponent>("Point Light Component", this->selectedEntity);
        drawComponentAdd<SpotLightComponent>("Spot Light Component", this->selectedEntity);
        drawComponentAdd<AmbientComponent>("Ambient Light Component", this->selectedEntity);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Remove Component"))
      {
        this->selectedEntity = entity;

        // Remove various components.
        drawComponentRemove<TransformComponent>("Transform Component", this->selectedEntity);
        drawComponentRemove<RenderableComponent>("Renderable Component", this->selectedEntity);
        drawComponentRemove<DirectionalLightComponent>("Directional Light Component", this->selectedEntity);
        drawComponentRemove<PointLightComponent>("Point Light Component", this->selectedEntity);
        drawComponentRemove<SpotLightComponent>("Spot Light Component", this->selectedEntity);
        drawComponentRemove<AmbientComponent>("Ambient Light Component", this->selectedEntity);

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

      // Check to see if we should delete the entity.
      if (ImGui::MenuItem("Delete Entity"))
        entityDeleted = true;

      ImGui::EndPopup();
    }

    // Open the list of attached components.
    if (opened)
    {
      this->drawComponentNodes(entity);
      ImGui::TreePop();
    }

    // Delete the entity at the end of the draw session.
    if (entityDeleted)
    {
      activeScene->deleteEntity(entity);
      if (this->selectedEntity == entity)
        this->selectedEntity = Entity();
    }
  }

  // Function for drawing sub-entities and components of an entity.
  void
  SceneGraphWindow::drawComponentNodes(Entity entity)
  {
    ImGuiTreeNodeFlags leafFlag = ImGuiTreeNodeFlags_Leaf
                                | ImGuiTreeNodeFlags_NoTreePushOnOpen;
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
  SceneGraphWindow::drawPropsWindow(bool &isOpen)
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

      drawComponentProperties<TransformComponent>("Transform Component",
        this->selectedEntity, [](auto& component)
      {
        Styles::drawVec3Controls("Translation", glm::vec3(0.0f), component.translation);
        glm::vec3 tEulerRotation = glm::degrees(component.rotation);
        Styles::drawVec3Controls("Rotation", glm::vec3(0.0f), tEulerRotation);
        component.rotation = glm::radians(tEulerRotation);
        Styles::drawVec3Controls("Shear", glm::vec3(1.0f), component.scale);
        Styles::drawFloatControl("Scale", 1.0f, component.scaleFactor);
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
        this->selectedEntity, [this](auto& component)
      {
        ImGui::PushID("DirectionalLight");
        ImGui::Checkbox("Cast Shadows", &component.castShadows);
        ImGui::ColorEdit3("Colour", &component.colour.r);
        Styles::drawFloatControl("Intensity", 0.0f, component.intensity,
                                 0.0f, 0.1f, 0.0f, 1.0f);
        ImGui::PopID();
      });

      drawComponentProperties<PointLightComponent>("Point Light Component",
        this->selectedEntity, [this](auto& component)
      {
        ImGui::PushID("PointLight");
        ImGui::Checkbox("Cast Shadows", &component.castShadows);
        ImGui::ColorEdit3("Colour", &component.colour.r);
        Styles::drawVec2Controls("Attenuation", glm::vec2(0.0f), component.attenuation);
        Styles::drawFloatControl("Intensity", 0.0f, component.intensity,
                                 0.0f, 0.1f, 0.0f, 1.0f);
        ImGui::PopID();
      });

      drawComponentProperties<SpotLightComponent>("Spot Light Component",
        this->selectedEntity, [this](auto& component)
      {
        ImGui::PushID("SpotLight");
        ImGui::Checkbox("Cast Shadows", &component.castShadows);
        ImGui::ColorEdit3("Colour", &component.colour.r);
        Styles::drawVec2Controls("Attenuation", glm::vec2(0.0f), component.attenuation);
        Styles::drawFloatControl("Intensity", 0.0f, component.intensity,
                                 0.0f, 0.1f, 0.0f, 1.0f);
        GLfloat innerAngle = glm::degrees(std::acos(component.innerCutoff));
        Styles::drawFloatControl("Inner Cutoff", 45.0f, innerAngle,
                                 0.0f, 0.1f, 0.0f, 360.0f);
        component.innerCutoff = std::cos(glm::radians(innerAngle));
        GLfloat outerAngle = glm::degrees(std::acos(component.outerCutoff));
        Styles::drawFloatControl("Outer Cutoff", 90.0f, outerAngle,
                                 0.0f, 0.1f, 0.0f, 360.0f);
        component.outerCutoff = std::cos(glm::radians(outerAngle));
        ImGui::PopID();
      });

      drawComponentProperties<AmbientComponent>("Ambient Light Component",
        this->selectedEntity, [this](auto& component)
      {
        bool attachEnvi = false;

        if (ImGui::Button("Load New Environment"))
          attachEnvi = true;

        if (component.ambient->hasEqrMap())
        {
          ImGui::Checkbox("Draw Blurred", &component.drawingMips);
          if (component.drawingMips)
          {
            ImGui::SliderFloat("Roughness", &component.roughness, 0.0f, 1.0f);
            component.ambient->setDrawingType(MapType::Prefilter);
          }
          else
            component.ambient->setDrawingType(MapType::Skybox);

          ImGui::DragFloat("Gamma", &component.gamma, 0.1f, 0.0f, 10.0f, "%.2f");
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
        // If it already has a mesh component, just modify the existing one.
        if (this->selectedEntity.hasComponent<RenderableComponent>())
        {
          auto& renderable = this->selectedEntity.getComponent<RenderableComponent>();
          renderable.model = modelAssets->getAsset(name);
          renderable.meshName = name;

          isOpen = false;
          this->selectedString = "";
        }
        else
        {
          this->selectedEntity.addComponent<RenderableComponent>
            (modelAssets->getAsset(name), name);

          isOpen = false;
          this->selectedString = "";
        }
      }
    }
    ImGui::End();

    if (!isOpen)
      this->selectedString = "";
  }
}
