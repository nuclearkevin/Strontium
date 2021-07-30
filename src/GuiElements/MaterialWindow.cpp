#include "GuiElements/MaterialWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"
#include "Serialization/YamlSerialization.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  MaterialWindow::MaterialWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , newMaterialName("")
  { }

  MaterialWindow::~MaterialWindow()
  { }

  // This is uglier than me, and more complicated than it needs to be.
  // TODO: Refactor and make it cleaner.
  void
  MaterialWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    auto modelAssets = AssetManager<Model>::getManager();
    auto materialAssets = AssetManager<Material>::getManager();

    float fontSize = ImGui::GetFontSize();
    static bool showTexWindow = false;
    static bool showMaterialWindow = false;
    static bool showNewMaterialWindow = false;
    static std::string selectedType, submesh;

    if (!this->selectedEntity)
      return;

    if (!this->selectedEntity.hasComponent<RenderableComponent>())
      return;

    ImGui::Begin("Materials", &isOpen);

    auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();
    if (rComponent)
    {
      auto& submeshes = modelAssets->getAsset(rComponent.meshName)->getSubmeshes();
      for (auto& pair : submeshes)
      {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth
                                 | ImGuiTreeNodeFlags_OpenOnArrow
                                 | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        bool opened = ImGui::TreeNodeEx((pair.first + "##" + std::to_string((unsigned long) pair.second.get())).c_str(), flags);
        if (!opened)
          continue;

        this->DNDMaterialTarget(pair.first);

        auto material = rComponent.materials.getMaterial(pair.second->getName());

        ImGui::Text("Material Name: %s", rComponent.materials.getMaterialHandle(pair.second->getName()).c_str());
        if (material->getFilepath() != "")
          ImGui::Text("Material Path: %s", material->getFilepath().c_str());

        // Material settings for the submesh.
        if (ImGui::Button(("Change Material##" + std::to_string((unsigned long) pair.second.get())).c_str()))
        {
          showMaterialWindow = true;
          submesh = pair.first;
        }

        ImGui::SameLine();
        if (ImGui::Button(("New Material##" + std::to_string((unsigned long) pair.second.get())).c_str()))
        {
          showNewMaterialWindow = true;
          submesh = pair.first;
        }

        ImGui::SameLine();
        if (ImGui::Button(("Save Material##" + std::to_string((unsigned long) pair.second.get())).c_str()))
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                       ".smtl"));
          this->selectedHandle = rComponent.materials.getMaterialHandle(pair.second->getName());
          this->fileSaveTarget = FileSaveTargets::TargetMaterial;
        }

        // Actual material properties.
        if (ImGui::CollapsingHeader(("Material Properties##" + std::to_string((unsigned long) pair.second.get())).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
          auto& materialPipeline = material->getPipeline();

          auto& uAlbedo = material->getVec3("uAlbedo");
          auto& uMetallic = material->getFloat("uMetallic");
          auto& uRoughness = material->getFloat("uRoughness");
          auto& uAO = material->getFloat("uAO");
          auto& uEmiss = material->getFloat("uEmiss");
          auto& uF0 = material->getFloat("uF0");

          // Draw all the associated texture maps for the entity.
          ImGui::Text("Albedo Map");
          ImGui::PushID("Albedo Button");
          if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D("albedoMap")->getID(),
                                 ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
          {
            showTexWindow = true;
            selectedType = "albedoMap";
            submesh = pair.first;
          }
          this->DNDTextureTarget(material, "albedoMap");
          auto endCursorPos = ImGui::GetCursorPos();
          ImGui::SameLine();
          ImGui::PopID();
          auto cursorPos = ImGui::GetCursorPos();
          ImGui::ColorEdit3("##Albedo", &uAlbedo.r);
          ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + fontSize + 8.0f));
          ImGui::Text("Emissivity");
          ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + 2 * fontSize + 12.0f));
          ImGui::SliderFloat("##Emiss", &uEmiss, 0.0f, 10.0f);
          ImGui::SetCursorPos(endCursorPos);

          ImGui::Text("Metallic Map");
          ImGui::PushID("Metallic Button");
          if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D("metallicMap")->getID(),
                                 ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
          {
            showTexWindow = true;
            selectedType = "metallicMap";
            submesh = pair.first;
          }
          this->DNDTextureTarget(material, "metallicMap");
          ImGui::PopID();
          ImGui::SameLine();
          ImGui::SliderFloat("##Metallic", &uMetallic, 0.0f, 1.0f);

          ImGui::Text("Roughness Map");
          ImGui::PushID("Roughness Button");
          if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D("roughnessMap")->getID(),
                                 ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
          {
            showTexWindow = true;
            selectedType = "roughnessMap";
            submesh = pair.first;
          }
          this->DNDTextureTarget(material, "roughnessMap");
          ImGui::PopID();
          ImGui::SameLine();
          ImGui::SliderFloat("##Roughness", &uRoughness, 0.01f, 1.0f);

          ImGui::Text("Ambient Occlusion Map");
          ImGui::PushID("Ambient Button");
          if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D("aOcclusionMap")->getID(),
                                 ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
          {
            showTexWindow = true;
            selectedType = "aOcclusionMap";
            submesh = pair.first;
          }
          this->DNDTextureTarget(material, "aOcclusionMap");
          ImGui::PopID();
          ImGui::SameLine();
          ImGui::SliderFloat("##AO", &uAO, 0.0f, 1.0f);

          ImGui::Text("Specular F0 Map");
          ImGui::PushID("F0 Button");
          if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D("specF0Map")->getID(),
                                 ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
          {
            showTexWindow = true;
            selectedType = "specF0Map";
            submesh = pair.first;
          }
          this->DNDTextureTarget(material, "specF0Map");
          ImGui::PopID();
          ImGui::SameLine();
          ImGui::SliderFloat("##F0", &uF0, 0.0f, 1.0f);

          ImGui::Text("Normal Map");
          ImGui::PushID("Normal Button");
          if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D("normalMap")->getID(),
                                 ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
          {
            showTexWindow = true;
            selectedType = "normalMap";
            submesh = pair.first;
          }
          this->DNDTextureTarget(material, "normalMap");
          ImGui::PopID();
        }
        ImGui::TreePop();
      }
      ImGui::End();
    }

    if (showTexWindow)
      this->drawTextureWindow(selectedType, submesh, showTexWindow);
    if (showMaterialWindow)
      this->drawMaterialWindow(submesh, showMaterialWindow);
    if (showNewMaterialWindow)
      this->drawNewMaterialWindow(submesh, showNewMaterialWindow);
  }

  void
  MaterialWindow::onUpdate(float dt, Shared<Scene> activeScene)
  { }

  void
  MaterialWindow::onEvent(Event &event)
  {
    switch(event.getType())
    {
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

      case EventType::SaveFileEvent:
      {
        auto saveEvent = *(static_cast<SaveFileEvent*>(&event));

        auto& path = saveEvent.getAbsPath();
        auto& name = saveEvent.getFileName();

        switch (this->fileSaveTarget)
        {
          case FileSaveTargets::TargetMaterial:
          {
            auto material = AssetManager<Material>::getManager()->getAsset(this->selectedHandle);

            YAMLSerialization::serializeMaterial(this->selectedHandle, path);
            this->fileSaveTarget = FileSaveTargets::TargetNone;
            this->selectedHandle = "";

            material->getFilepath() = path;
            break;
          }
        }

        break;
      }

      // Process a file loading event. Using enum barriers to prevent files from
      // being improperly loaded when this window didn't dispatch the event.
      case EventType::LoadFileEvent:
      {
        if (!this->selectedEntity)
          return;

        auto loadEvent = *(static_cast<LoadFileEvent*>(&event));
        auto& path = loadEvent.getAbsPath();
        auto& name = loadEvent.getFileName();

        switch (this->fileTargets)
        {
          case FileLoadTargets::TargetTexture:
          {
            Texture2D::loadImageAsync(path);
            this->selectedMatTex.first->attachSampler2D(this->selectedMatTex.second, name);

            this->selectedMatTex = std::make_pair(nullptr, "");
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
  MaterialWindow::drawTextureWindow(const std::string &type, const std::string &submesh,
                                    bool &isOpen)
  {
    if (!this->selectedEntity)
      return;

    auto textureCache = AssetManager<Texture2D>::getManager();

    Material* material = this->selectedEntity.getComponent<RenderableComponent>()
                                             .materials.getMaterial(submesh);

    ImGui::Begin("Select Texture", &isOpen);
    if (ImGui::Button("Load New Texture"))
    {
      EventDispatcher* dispatcher = EventDispatcher::getInstance();
      dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                   ".jpg,.tga,.png"));

      this->selectedMatTex = std::make_pair(material, type);
      this->fileTargets = FileLoadTargets::TargetTexture;

      isOpen = false;
    }

    for (auto& handle : textureCache->getStorage())
    {
      ImGuiTreeNodeFlags flags = ((this->selectedHandle == handle) ? ImGuiTreeNodeFlags_Selected : 0);
      flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_OpenOnDoubleClick;

      auto texture = textureCache->getAsset(handle);
      ImGui::Image((ImTextureID) (unsigned long) texture->getID(),
                   ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        material->attachSampler2D(type, handle);
        isOpen = false;
        this->selectedHandle = "";
      }

      ImGui::SameLine();
      bool opened = ImGui::TreeNodeEx((void*) (sizeof(char) * handle.size()), flags, handle.c_str());

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        material->attachSampler2D(type, handle);
        isOpen = false;
        this->selectedHandle = "";
      }
    }
    ImGui::End();

    if (!isOpen)
      this->selectedHandle = "";
  }

  void
  MaterialWindow::drawMaterialWindow(const std::string &submesh, bool &isOpen)
  {
    auto materialAssets = AssetManager<Material>::getManager();

    auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();

    ImGui::Begin("Select Material", &isOpen);
    for (auto& handle : materialAssets->getStorage())
    {
      ImGuiTreeNodeFlags flags = ((this->selectedHandle == handle) ? ImGuiTreeNodeFlags_Selected : 0);
      flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_OpenOnDoubleClick;

      bool opened = ImGui::TreeNodeEx((void*) (sizeof(char) * handle.size()), flags, handle.c_str());

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        rComponent.materials.swapMaterial(submesh, handle);
        isOpen = false;
      }
    }

    ImGui::End();
  }

  void
  MaterialWindow::drawNewMaterialWindow(const std::string &submesh, bool &isOpen)
  {
    char nameBuffer[256];
    memset(nameBuffer, 0, sizeof(nameBuffer));
    std::strncpy(nameBuffer, this->newMaterialName.c_str(), sizeof(nameBuffer));

    ImGui::Begin("New Material Name", &isOpen, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Name:");
    if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer)))
      this->newMaterialName = std::string(nameBuffer);

    if (ImGui::Button("Create"))
    {
      auto materialAssets = AssetManager<Material>::getManager();
      auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();

      materialAssets->attachAsset(this->newMaterialName, new Material());
      rComponent.materials.swapMaterial(submesh, this->newMaterialName);

      this->newMaterialName = "";
      isOpen = false;
    }
    ImGui::End();
  }

  void
  MaterialWindow::DNDTextureTarget(Material* material, const std::string &selectedType)
  {
    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
      {
        this->selectedMatTex = std::make_pair(material, selectedType);
        this->loadDNDTextureAsset((char*) payload->Data);
      }

      ImGui::EndDragDropTarget();
    }
  }

  void
  MaterialWindow::DNDMaterialTarget(const std::string &subMesh)
  {
    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
        this->loadDNDMaterial((char*) payload->Data, subMesh);

      ImGui::EndDragDropTarget();
    }
  }

  void
  MaterialWindow::loadDNDTextureAsset(const std::string &filepath)
  {
    std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
    std::string filetype = filename.substr(filename.find_last_of('.'));

    if (filetype == ".jpg" || filetype == ".tga" || filetype == ".png")
    {
      Texture2D::loadImageAsync(filepath);
      this->selectedMatTex.first->attachSampler2D(this->selectedMatTex.second, filename);
    }

    this->selectedMatTex = std::make_pair(nullptr, "");
  }

  void
  MaterialWindow::loadDNDMaterial(const std::string &filepath, const std::string &subMesh)
  {
    auto materialAssets = AssetManager<Material>::getManager();

    std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
    std::string filetype = filename.substr(filename.find_last_of('.'));

    if (!(filetype == ".smtl"))
      return;

    AssetHandle handle;
    if (!YAMLSerialization::deserializeMaterial(filepath, handle))
      return;

    materialAssets->getAsset(handle)->getFilepath() = filepath;

    auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();
    rComponent.materials.swapMaterial(subMesh, handle);
  }
}
