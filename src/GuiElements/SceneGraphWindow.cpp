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
    this->openModel = false;
  }

  void
  SceneGraphWindow::onDetach()
  { }

  void
  SceneGraphWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    bool openModelDialoge = this->openModel;

    ImGui::Begin("Scene Graph", &isOpen);
    activeScene->sceneECS.each([&](auto entityID)
    {
      // Fetch the entity and its tag component.
      Entity current = Entity(entityID, activeScene.get());
      this->drawEntityNode(current, activeScene);
    });

    // Right-click on blank space to create a new entity.
		if (ImGui::BeginPopupContextWindow(0, 1, false))
		{
			if (ImGui::MenuItem("Create New Entity"))
				activeScene->createEntity();

			ImGui::EndPopup();
		}

    // Load in a model and give it to the entity as a renderable component
    // alongside the PBR shader.
    if (this->openModel)
      ImGui::OpenPopup("Load Mesh");

    if (this->fileHandler.showFileDialog("Load Mesh",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".obj"))
    {
      auto shaderCache = AssetManager<Shader>::getManager();
      auto meshAssets = AssetManager<Mesh>::getManager();

      std::string name = this->fileHandler.selected_fn;
      std::string path = this->fileHandler.selected_path;

      this->selectedEntity.addComponent<RenderableComponent>
        (meshAssets->loadAssetFile(path, name), shaderCache->getAsset("pbr"));
      this->selectedEntity.addComponent<TransformComponent>();
      this->openModel = false;
    }

    // Deselect the entity if it isn't hovered over, this causes issues later.
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !this->openModel)
			this->selectedEntity = Entity();

    ImGui::End();

    // The property panel for an entity.
    ImGui::Begin("Entity Properties", &isOpen);
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
        auto& tTranslation = this->selectedEntity.getComponent<TransformComponent>().translation;
        auto& tTotation = this->selectedEntity.getComponent<TransformComponent>().rotation;
        auto& tScale = this->selectedEntity.getComponent<TransformComponent>().scaleFactor;

        ImGui::SliderFloat3("Translation", &tTranslation.x, -10.0f, 10.0f);
        ImGui::SliderFloat3("Rotation", &tTotation.x, -10.0f, 10.0f);
        ImGui::SliderFloat("Scale", &tScale, 0.01f, 10.0f);
      }
    }
    ImGui::End();
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
    bool openModel;
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
        if (ImGui::MenuItem("Renderable Component") &&
            !entity.hasComponent<RenderableComponent>())
        {
          this->openModel = true;
        }
        ImGui::EndMenu();
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
}
