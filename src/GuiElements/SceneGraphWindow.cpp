#include "GuiElements/SceneGraphWindow.h"

// STL includes.
#include <cstring>

// Project includes.
#include "Core/AssetManager.h"

namespace SciRenderer
{
  void
  SceneGraphWindow::onAttach()
  {
    this->attachMesh = false;
    this->propsWindow = true;
    this->selectedMesh = "";
  }

  void
  SceneGraphWindow::onDetach()
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
        auto meshAssets = AssetManager<Mesh>::getManager();

				auto bunny = activeScene->createEntity();
        bunny.addComponent<TransformComponent>();
        bunny.addComponent<RenderableComponent>
          (meshAssets->loadAssetFile("./res/models/bunny.obj", "bunny.obj"),
           shaderCache->getAsset("pbr"), "bunny.obj", "./res/models/bunny.obj", "pbr");

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

    // Menu with entity properties.
    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
      this->selectedEntity = entity;
      if (ImGui::BeginMenu("Attach Component"))
      {
        if (!entity.hasComponent<TransformComponent>())
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

        if (!entity.hasComponent<RenderableComponent>())
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

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Remove Component"))
      {
        if (entity.hasComponent<TransformComponent>())
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

        if (entity.hasComponent<RenderableComponent>())
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
          auto& tScale = this->selectedEntity.getComponent<TransformComponent>().scaleFactor;

          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
          ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
          ImGui::DragFloat("##tX", &tTranslation.x, 0.1f, 0.0f, 0.0f, "%.2f");
          ImGui::PopItemWidth();
          ImGui::SameLine();
          ImGui::DragFloat("##tY", &tTranslation.y, 0.1f, 0.0f, 0.0f, "%.2f");
          ImGui::PopItemWidth();
          ImGui::SameLine();
          ImGui::DragFloat("##tZ", &tTranslation.z, 0.1f, 0.0f, 0.0f, "%.2f");
          ImGui::PopItemWidth();
          ImGui::SameLine();
          ImGui::Text("Translation");

          ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
          ImGui::DragFloat("##rX", &tRotation.x, 0.1f, 0.0f, 0.0f, "%.2f");
          ImGui::PopItemWidth();
          ImGui::SameLine();
          ImGui::DragFloat("##rY", &tRotation.y, 0.1f, 0.0f, 0.0f, "%.2f");
          ImGui::PopItemWidth();
          ImGui::SameLine();
          ImGui::DragFloat("##rZ", &tRotation.z, 0.1f, 0.0f, 0.0f, "%.2f");
          ImGui::PopItemWidth();
          ImGui::SameLine();
          ImGui::Text("Rotation");

          ImGui::DragFloat("Scale", &tScale, 0.1f, 0.0f, 0.0f, "%.2f");
          ImGui::PopStyleVar();
        }
      }

      if (this->selectedEntity.hasComponent<RenderableComponent>())
      {
        if (ImGui::CollapsingHeader("Renderable Component"))
        {
          auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();
          ImGui::Text((std::string("Mesh: ") + rComponent.meshName).c_str());
          ImGui::Text((std::string("    Path: ") + rComponent.meshPath).c_str());
          ImGui::Text((std::string("Shader: ") + rComponent.shaderName).c_str());

          if (ImGui::Button("Select New Mesh"))
            this->attachMesh = true;
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
    auto meshAssets = AssetManager<Mesh>::getManager();
    auto shaderCache = AssetManager<Shader>::getManager();

    ImGui::Begin("Mesh Assets", &this->attachMesh);

    // Button to load in a new mesh asset.
    if (ImGui::Button("Load New Mesh"))
      openModelDialog = true;

    // Button to use one of the loaded meshes for the renderable component.
    ImGui::SameLine();
    if (this->selectedMesh != "")
    {
      if (ImGui::Button("Use Selected##selectedModelbutton"))
      {
        // Remove the mesh component if it has one.
        if (this->selectedEntity.hasComponent<RenderableComponent>())
        {
          auto& renderable = this->selectedEntity.getComponent<RenderableComponent>();
          renderable.mesh = meshAssets->getAsset(this->selectedMesh);
          renderable.meshName = this->selectedMesh;
          renderable.meshPath = renderable.mesh->getFilepath();

          this->attachMesh = false;
          this->selectedMesh = "";
        }
        else
        {
          this->selectedEntity.addComponent<RenderableComponent>
            (meshAssets->getAsset(this->selectedMesh), shaderCache->getAsset("pbr"),
             this->selectedMesh, meshAssets->getAsset(this->selectedMesh)->getFilepath(), "pbr");

          this->attachMesh = false;
          this->selectedMesh = "";
        }
      }
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("Use Selected##selectedModelbutton");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }

    // Loop over all the loaded models and display each for selection.
    for (auto& name : meshAssets->getStorage())
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
    // alongside the PBR shader.
    if (openModelDialog)
      ImGui::OpenPopup("Load Mesh");

    if (this->fileHandler.showFileDialog("Load Mesh",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".obj"))
    {
      auto shaderCache = AssetManager<Shader>::getManager();
      auto meshAssets = AssetManager<Mesh>::getManager();

      std::string name = this->fileHandler.selected_fn;
      std::string path = this->fileHandler.selected_path;

      if (this->selectedEntity.hasComponent<RenderableComponent>())
      {
        auto& renderable = this->selectedEntity.getComponent<RenderableComponent>();
        renderable.mesh = meshAssets->loadAssetFile(path, name);
        renderable.meshName = name;
        renderable.meshPath = renderable.mesh->getFilepath();

        this->attachMesh = false;
        this->selectedMesh = "";
      }
      else
      {
        this->selectedEntity.addComponent<RenderableComponent>
          (meshAssets->loadAssetFile(path, name), shaderCache->getAsset("pbr"), name, path, "pbr");

        this->attachMesh = false;
        this->selectedMesh = "";
      }
    }
  }
}
