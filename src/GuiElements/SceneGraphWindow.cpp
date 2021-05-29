#include "GuiElements/SceneGraphWindow.h"

// STL includes.
#include <cstring>

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

namespace SciRenderer
{
  SceneGraphWindow::SceneGraphWindow()
    : GuiWindow()
    , attachMesh(false)
    , attachEnvi(false)
    , propsWindow(true)
    , selectedMesh("")
  { }

  SceneGraphWindow::~SceneGraphWindow()
  { }

  void
  SceneGraphWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    ImGui::Begin("Scene Graph", &isOpen);

    if (!this->propsWindow)
    {
      this->propsWindow = this->propsWindow || ImGui::Button("Open Properties Window");
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("Open Properties Window");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }

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
      if (ImGui::MenuItem("Create New Debug Bunny"))
      {
        auto shaderCache = AssetManager<Shader>::getManager();
        auto modelAssets = AssetManager<Model>::getManager();

				auto bunny = activeScene->createEntity();
        bunny.addComponent<TransformComponent>();
        bunny.addComponent<RenderableComponent>
          (modelAssets->loadAssetFile("./res/models/bunny.obj", "bunny.obj"),
           shaderCache->getAsset("pbr_shader"), "bunny.obj", "./res/models/bunny.obj", "pbr_shader");

        auto& tag = bunny.getComponent<NameComponent>();
        tag.name = "Debug Bunny";

        auto& transform = bunny.getComponent<TransformComponent>();
        transform.scaleFactor = 10.0f;
      }

			ImGui::EndPopup();
		}

    // Deselect the entity if it isn't hovered over.
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !this->attachMesh)
			this->selectedEntity = Entity();

    ImGui::End();

    if (this->propsWindow)
      this->drawPropsWindow();
    if (this->attachMesh)
      this->drawMeshWindow();
    if (this->attachEnvi)
      this->drawEnviWindow();
  }

  void
  SceneGraphWindow::onUpdate(float dt)
  {

  }

  void
  SceneGraphWindow::onEvent(Event &event)
  {

  }

  void
  SceneGraphWindow::drawEntityNode(Entity entity, Shared<Scene> activeScene)
  {
    auto& nameTag = entity.getComponent<NameComponent>().name;

    // Draw the entity + components as a treenode.
    ImGuiTreeNodeFlags flags = ((this->selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, nameTag.c_str());

    // Set the new selected entity.
    if (ImGui::IsItemClicked())
      this->selectedEntity = entity;

    // Menu with entity properties. Allows the addition and deletion of
    // components, copying of the entity and deletion of the entity.
    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
      this->selectedEntity = entity;
      if (ImGui::BeginMenu("Attach Component"))
      {
        if (!this->selectedEntity.hasComponent<TransformComponent>())
        {
          if (ImGui::MenuItem("Transform Component"))
            this->selectedEntity.addComponent<TransformComponent>();
        }
        else
        {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
          ImGui::MenuItem("Transform Component");
          ImGui::PopItemFlag();
          ImGui::PopStyleVar();
        }

        if (!this->selectedEntity.hasComponent<RenderableComponent>())
        {
          if (ImGui::MenuItem("Renderable Component"))
            this->attachMesh = true;
        }
        else
        {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
          ImGui::MenuItem("Renderable Component");
          ImGui::PopItemFlag();
          ImGui::PopStyleVar();
        }

        // For now, only allow a single AmbientComponent per scene.
        if (!this->selectedEntity.hasComponent<AmbientComponent>()
            && activeScene->sceneECS.size<AmbientComponent>() == 0)
        {
          if (ImGui::MenuItem("Ambient Light Component"))
            this->selectedEntity.addComponent<AmbientComponent>();
        }
        else
        {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
          ImGui::MenuItem("Ambient Light Component");
          ImGui::PopItemFlag();
          ImGui::PopStyleVar();
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Remove Component"))
      {
        if (this->selectedEntity.hasComponent<TransformComponent>())
        {
          if (ImGui::MenuItem("Transform Component"))
            this->selectedEntity.removeComponent<TransformComponent>();
        }
        else
        {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
          ImGui::MenuItem("Transform Component");
          ImGui::PopItemFlag();
          ImGui::PopStyleVar();
        }

        if (this->selectedEntity.hasComponent<RenderableComponent>())
        {
          if (ImGui::MenuItem("Renderable Component"))
            this->selectedEntity.removeComponent<RenderableComponent>();
        }
        else
        {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
          ImGui::MenuItem("Renderable Component");
          ImGui::PopItemFlag();
          ImGui::PopStyleVar();
        }

        if (this->selectedEntity.hasComponent<AmbientComponent>())
        {
          if (ImGui::MenuItem("Ambient Light Component"))
            this->selectedEntity.removeComponent<AmbientComponent>();
        }
        else
        {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
          ImGui::MenuItem("Ambient Light Component");
          ImGui::PopItemFlag();
          ImGui::PopStyleVar();
        }

        ImGui::EndMenu();
      }

      if (ImGui::MenuItem("Create Copy of Entity"))
      {
        auto newEntity = activeScene->createEntity();

        if (entity.hasComponent<NameComponent>())
        {
          auto& comp = newEntity.getComponent<NameComponent>();
          comp = entity.getComponent<NameComponent>();
        }

        if (entity.hasComponent<TransformComponent>())
        {
          auto& comp = newEntity.addComponent<TransformComponent>();
          comp = entity.getComponent<TransformComponent>();
        }

        if (entity.hasComponent<RenderableComponent>())
        {
          auto& comp = newEntity.addComponent<RenderableComponent>();
          comp = entity.getComponent<RenderableComponent>();
        }
      }

      // Check to see if we should delete the entity.
      if (ImGui::MenuItem("Delete Entity"))
        entityDeleted = true;

      ImGui::EndPopup();
    }

    // Open the list of attached components.
    if (opened)
    {
      // Display components here.
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
  SceneGraphWindow::drawComponentNodes()
  {

  }

  // The property panel for an entity.
  void
  SceneGraphWindow::drawPropsWindow()
  {
    ImGui::Begin("Entity Properties", &(this->propsWindow));
    if (this->selectedEntity)
    {
      if (this->selectedEntity.hasComponent<NameComponent>())
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
      }

      if (this->selectedEntity.hasComponent<TransformComponent>())
      {
        if (ImGui::CollapsingHeader("Transform Component"))
        {
          auto& tTranslation = this->selectedEntity.getComponent<TransformComponent>().translation;
          auto& tRotation = this->selectedEntity.getComponent<TransformComponent>().rotation;
          auto& tShear = this->selectedEntity.getComponent<TransformComponent>().scale;
          auto& tScale = this->selectedEntity.getComponent<TransformComponent>().scaleFactor;

          Styles::drawVec3Controls("Translation", glm::vec3(0.0f), tTranslation);
          Styles::drawVec3Controls("Rotation", glm::vec3(0.0f), tRotation);
          Styles::drawVec3Controls("Shear", glm::vec3(0.0f), tShear);
          Styles::drawFloatControl("Scale", 1.0f, tScale);
        }
      }

      if (this->selectedEntity.hasComponent<RenderableComponent>())
      {
        if (ImGui::CollapsingHeader("Renderable Component"))
        {
          auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();

          ImGui::Text((std::string("Mesh: ") + rComponent.meshName).c_str());
          ImGui::Text((std::string("    Path: ") + rComponent.meshPath).c_str());
          if (ImGui::Button("Select New Mesh"))
            this->attachMesh = true;

          ImGui::Text((std::string("Shader: ") + rComponent.shaderName).c_str());
          ImGui::Text((std::string("Shader Info: ") + rComponent.shader->getInfoString()).c_str());
        }
      }

      if (this->selectedEntity.hasComponent<AmbientComponent>())
      {
        if (ImGui::CollapsingHeader("Ambient Light Component"))
        {
          auto& ambient = this->selectedEntity.getComponent<AmbientComponent>();
          if (ImGui::Button("Load New Environment"))
            this->attachEnvi = true;

          if (ambient.ambient->hasEqrMap())
          {
            ImGui::Checkbox("Draw Blurred", &ambient.drawingMips);
            if (ambient.drawingMips)
            {
              ImGui::SliderFloat("Roughness", &ambient.roughness, 0.0f, 1.0f);
              ambient.ambient->setDrawingType(MapType::Prefilter);
            }
            else
              ambient.ambient->setDrawingType(MapType::Skybox);
            ImGui::DragFloat("Gamma", &ambient.gamma, 0.1f, 0.0f, 10.0f, "%.2f");
          }
        }
      }
    }
    ImGui::End();
  }

  // Draw the mesh selection window.
  void
  SceneGraphWindow::drawMeshWindow()
  {
    bool openModelDialog = false;

    // Get the asset caches.
    auto modelAssets = AssetManager<Model>::getManager();
    auto shaderCache = AssetManager<Shader>::getManager();

    ImGui::Begin("Mesh Assets", &this->attachMesh);

    // Button to load in a new mesh asset.
    if (ImGui::Button("Load New Mesh"))
      openModelDialog = true;

    // Button to use one of the loaded meshes for the renderable component.
    ImGui::SameLine();
    if (this->selectedMesh != "")
    {
      if (ImGui::Button("Use Selected Mesh##selectedModelbutton"))
      {
        // If it already has a mesh component, just modify the existing one.
        if (this->selectedEntity.hasComponent<RenderableComponent>())
        {
          auto& renderable = this->selectedEntity.getComponent<RenderableComponent>();
          renderable.model = modelAssets->getAsset(this->selectedMesh);
          renderable.meshName = this->selectedMesh;
          renderable.meshPath = renderable.model->getFilepath();

          this->attachMesh = false;
          this->selectedMesh = "";
        }
        else
        {
          this->selectedEntity.addComponent<RenderableComponent>
            (modelAssets->getAsset(this->selectedMesh), shaderCache->getAsset("pbr_shader"),
             this->selectedMesh, modelAssets->getAsset(this->selectedMesh)->getFilepath(), "pbr_shader");

          this->attachMesh = false;
          this->selectedMesh = "";
        }
      }
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("Use Selected Mesh##selectedModelbutton");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }

    // Loop over all the loaded models and display each for selection.
    for (auto& name : modelAssets->getStorage())
    {
      ImGuiTreeNodeFlags flags = ((this->selectedMesh == name) ? ImGuiTreeNodeFlags_Selected : 0);
      flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_Bullet;

      bool opened = ImGui::TreeNodeEx((void*) (sizeof(char) * name.size()), flags, name.c_str());

      if (ImGui::IsItemClicked())
        this->selectedMesh = name;
    }
    ImGui::End();

    // Load in a model and give it to the entity as a renderable component
    // alongside the pbr_shader shader.
    if (openModelDialog)
      ImGui::OpenPopup("Load Mesh");

    if (this->fileHandler.showFileDialog("Load Mesh",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".obj,.FBX,.fbx"))
    {
      auto shaderCache = AssetManager<Shader>::getManager();
      auto modelAssets = AssetManager<Model>::getManager();

      std::string name = this->fileHandler.selected_fn;
      std::string path = this->fileHandler.selected_path;

      // If it already has a mesh component, just modify the existing one.
      if (this->selectedEntity.hasComponent<RenderableComponent>())
      {
        auto& renderable = this->selectedEntity.getComponent<RenderableComponent>();
        renderable.model = modelAssets->loadAssetFile(path, name);
        renderable.meshName = name;
        renderable.meshPath = renderable.model->getFilepath();

        this->attachMesh = false;
        this->selectedMesh = "";
      }
      else
      {
        this->selectedEntity.addComponent<RenderableComponent>
          (modelAssets->loadAssetFile(path, name), shaderCache->getAsset("pbr_shader"), name, path, "pbr_shader");

        this->attachMesh = false;
        this->selectedMesh = "";
      }
    }
  }

  void
  SceneGraphWindow::drawEnviWindow()
  {
    bool openEnviDialog = false;

    ImGui::OpenPopup("Load Environment Map");

    if (this->fileHandler.showFileDialog("Load Environment Map",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".hdr"))
    {
      std::string name = this->fileHandler.selected_fn;
      std::string path = this->fileHandler.selected_path;

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

      this->attachEnvi = false;
    }
  }
}
