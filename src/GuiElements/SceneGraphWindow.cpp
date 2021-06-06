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
    , showMaterialInfo(true)
    , selectedString("")
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
          (modelAssets->loadAssetFile("./res/models/bunnyNew.obj", "bunnyNew.obj"),
           "bunnyNew.obj");

        auto& tag = bunny.getComponent<NameComponent>();
        tag.name = "Debug Bunny";
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
    if (this->showMaterialInfo)
      this->drawMaterialWindow();
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

          glm::vec3 tEulerRotation = glm::vec3(glm::degrees(tRotation.x),
                                               glm::degrees(tRotation.y),
                                               glm::degrees(tRotation.z));

          Styles::drawVec3Controls("Translation", glm::vec3(0.0f), tTranslation);
          Styles::drawVec3Controls("Rotation", glm::vec3(0.0f), tEulerRotation);
          Styles::drawVec3Controls("Shear", glm::vec3(1.0f), tShear);
          Styles::drawFloatControl("Scale", 1.0f, tScale);

          tRotation = glm::vec3(glm::radians(tEulerRotation.x),
                                glm::radians(tEulerRotation.y),
                                glm::radians(tEulerRotation.z));
        }
      }

      if (this->selectedEntity.hasComponent<RenderableComponent>())
      {
        if (ImGui::CollapsingHeader("Renderable Component"))
        {
          auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();

          ImGui::Text((std::string("Mesh: ") + rComponent.meshName).c_str());
          if (ImGui::Button("Select New Mesh"))
            this->attachMesh = true;

          ImGui::Checkbox("Show Material Window", &this->showMaterialInfo);
        }
      }

      if (this->selectedEntity.hasComponent<AmbientComponent>())
      {
        if (ImGui::CollapsingHeader("Ambient Light Component"))
        {
          auto& ambient = this->selectedEntity.getComponent<AmbientComponent>();
          if(ImGui::Button("Load New Environment"))
          {
            this->attachEnvi = true;
          }

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

  // Draw the material info in a separate window to avoid cluttering.
  void
  SceneGraphWindow::drawMaterialWindow()
  {
    if (this->showMaterialInfo && this->selectedEntity)
    {
      if (this->selectedEntity.hasComponent<RenderableComponent>())
      {
        auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();
        for (auto& pair : rComponent.model->getSubmeshes())
        {
          ImGui::Begin("Materials", &this->showMaterialInfo);

          static bool showShaderInfo = false;
          static bool showTexWindow = false;
          static std::string selectedType;
          std::string selectedMeshName = pair.first;

          if (ImGui::CollapsingHeader((pair.first + "##" + std::to_string((unsigned long) pair.second.get())).c_str()))
          {
            auto material = rComponent.materials.getMaterial(pair.second);
            auto& uAlbedo = material->getVec3("uAlbedo");
            auto& uMetallic = material->getFloat("uMetallic");
            auto& uRoughness = material->getFloat("uRoughness");
            auto& uAO = material->getFloat("uAO");

            ImGui::Checkbox("Show Shader Info", &showShaderInfo);

            // Draw all the associated texture maps for the entity.
            ImGui::Text("Albedo Map");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("albedoMap")->getID(),
                         ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "albedoMap";
            }
            ImGui::SameLine();
            ImGui::ColorEdit3("##Albedo", &uAlbedo.r);

            ImGui::Text("Metallic Map");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("metallicMap")->getID(),
                         ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "metallicMap";
            }
            ImGui::SameLine();
            ImGui::SliderFloat("##Metallic", &uMetallic, 0.0f, 1.0f);

            ImGui::Text("Roughness Map");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("roughnessMap")->getID(),
                         ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "roughnessMap";
            }
            ImGui::SameLine();
            ImGui::SliderFloat("##Roughness", &uRoughness, 0.0f, 1.0f);

            ImGui::Text("Ambient Occlusion Map");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("aOcclusionMap")->getID(),
                         ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "aOcclusionMap";
            }
            ImGui::SameLine();
            ImGui::SliderFloat("##AO", &uAO, 0.0f, 1.0f);

            ImGui::Text("Normal Map");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("normalMap")->getID(),
                         ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "normalMap";
            }
          }
          ImGui::End();

          if (showTexWindow)
            this->drawTextureWindow(selectedType, pair.second, showTexWindow);
          if (showShaderInfo)
            this->drawShaderInfoWindow(showShaderInfo);
        }
      }
    }
  }

  // Draw the renderable component shader information. Its in another window
  // to make things easier to view.
  void
  SceneGraphWindow::drawShaderInfoWindow(bool &isOpen)
  {
    if (this->selectedEntity)
    {
      if (this->selectedEntity.hasComponent<RenderableComponent>())
      {
        auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();
        for (auto& pair : rComponent.model->getSubmeshes())
        {
          auto material = rComponent.materials.getMaterial(pair.second);
          ImGui::Begin("Shader Info", &isOpen);
          if (ImGui::CollapsingHeader(pair.first.c_str()))
            ImGui::Text((std::string("Shader Info: ") +
                        material->getShader()->getInfoString()).c_str());
          ImGui::End();
        }
      }
    }
  }

  // Draw the mesh selection window.
  void
  SceneGraphWindow::drawMeshWindow()
  {
    bool openModelDialogue = false;

    // Get the asset caches.
    auto modelAssets = AssetManager<Model>::getManager();
    auto shaderCache = AssetManager<Shader>::getManager();

    ImGui::Begin("Mesh Assets", &this->attachMesh);

    // Button to load in a new mesh asset.
    if (ImGui::Button("Load New Mesh"))
      openModelDialogue = true;

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

          this->attachMesh = false;
          this->selectedString = "";
        }
        else
        {
          this->selectedEntity.addComponent<RenderableComponent>
            (modelAssets->getAsset(name), name);

          this->attachMesh = false;
          this->selectedString = "";
        }
      }
    }
    ImGui::End();

    // Load in a model and give it to the entity as a renderable component
    // alongside the pbr_shader shader.
    if (openModelDialogue)
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

        this->attachMesh = false;
        this->selectedString = "";
      }
      else
      {
        this->selectedEntity.addComponent<RenderableComponent>
          (modelAssets->loadAssetFile(path, name), name);

        this->attachMesh = false;
        this->selectedString = "";
      }
    }

    if (!this->attachMesh)
      this->selectedString = "";
  }

  void
  SceneGraphWindow::drawEnviWindow()
  {
    if (this->attachEnvi)
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

  // Texture selection window.
  void
  SceneGraphWindow::drawTextureWindow(const std::string &type, Shared<Mesh> submesh, bool &isOpen)
  {
    auto textureCache = AssetManager<Texture2D>::getManager();

    Material* material =
      this->selectedEntity.getComponent<RenderableComponent>().materials.getMaterial(submesh);

    bool openTextureDialoge = false;

    ImGui::Begin("Select Texture", &isOpen);

    if (ImGui::Button("Load New Texture"))
      openTextureDialoge = true;

    for (auto& name : textureCache->getStorage())
    {
      ImGuiTreeNodeFlags flags = ((this->selectedString == name) ? ImGuiTreeNodeFlags_Selected : 0);
      flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_OpenOnDoubleClick;

      auto texture = textureCache->getAsset(name);
      ImGui::Image((ImTextureID) (unsigned long) texture->getID(),
                   ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        material->attachTexture2D(texture, type);
        isOpen = false;
        this->selectedString = "";
      }

      ImGui::SameLine();
      bool opened = ImGui::TreeNodeEx((void*) (sizeof(char) * name.size()), flags, name.c_str());

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        material->attachTexture2D(texture, type);
        isOpen = false;
        this->selectedString = "";
      }
    }
    ImGui::End();

    // Load in a new texture and attach it to the material.
    if (openTextureDialoge)
      ImGui::OpenPopup("Load Texture");

    if (this->fileHandler.showFileDialog("Load Texture",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".jpg,.tga,.png"))
    {
      std::string name = this->fileHandler.selected_fn;
      std::string path = this->fileHandler.selected_path;

      Texture2D* tex = Texture2D::loadTexture2D(path);

      material->attachTexture2D(tex, type);
      isOpen = false;
      this->selectedString = "";
    }

    if (!isOpen)
      this->selectedString = "";
  }
}
