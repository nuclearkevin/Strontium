#include "GuiElements/ModelWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"
#include "Serialization/YamlSerialization.h"
#include "Utils/AsyncAssetLoading.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// Other includes.
#include "glm/gtx/string_cast.hpp"

namespace Strontium
{
  ModelWindow::ModelWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , newMaterialName("")
    , searched("")
    , selectedNode(nullptr)
    , selectedAniNode(nullptr)
    , selectedSubMesh(nullptr)
  { }

  ModelWindow::~ModelWindow()
  { }

  // This is uglier than me, and more complicated than it needs to be.
  // TODO: Refactor and make it cleaner.
  void
  ModelWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    auto modelAssets = AssetManager<Model>::getManager();
    auto materialAssets = AssetManager<Material>::getManager();

    float fontSize = ImGui::GetFontSize();
    static bool showTexWindow = false;
    static bool showMaterialWindow = false;
    static std::string selectedType, submeshHandle;

    if (!this->selectedEntity)
      return;

    if (!this->selectedEntity.hasComponent<RenderableComponent>())
      return;

    ImGui::Begin("Model Information", &isOpen);

    auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();
    auto model = modelAssets->getAsset(rComponent.meshName);
    if (rComponent)
    {
      ImGui::Text("Model name: %s", rComponent.meshName.c_str());
      ImGui::Text("Model path: %s", model->getFilepath().c_str());
      auto min = model->getMinPos();
      auto max = model->getMaxPos();
      ImGui::Text("AABB: \n\tMin: (%f, %f, %f) \n\tMax: (%f, %f, %f)", min.x, min.y, min.z, max.x, max.y, max.z);
      if (ImGui::CollapsingHeader("Scene"))
      {
        ImGui::Indent();
        ImGui::Text("Root Node: %s", model->getRootNode().name.c_str());
        ImGui::Separator();
        ImGui::Text("Scene Nodes");
        ImGui::Separator();

        if (this->selectedNode)
        {
          if (ImGui::BeginCombo("##combo", this->selectedNode->name.c_str()))
          {
            for (auto& sceneNode : model->getSceneNodes())
            {
              bool isSelected = (&sceneNode.second == this->selectedNode);

              if (ImGui::Selectable(sceneNode.first.c_str(), isSelected))
                this->selectedNode = (&sceneNode.second);

              if (isSelected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }

          if (ImGui::CollapsingHeader("Child Nodes"))
          {
            for (auto& sceneNodeChild : this->selectedNode->childNames)
              ImGui::Text("%s", sceneNodeChild.c_str());
          }
        }
        else
        {
          if (ImGui::BeginCombo("##combo", ""))
          {
            for (auto& sceneNode : model->getSceneNodes())
            {
              bool isSelected = (&sceneNode.second == this->selectedNode);

              if (ImGui::Selectable(sceneNode.first.c_str(), isSelected))
                this->selectedNode = (&sceneNode.second);

              if (isSelected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }
        }
        ImGui::Unindent();
      }
      if (ImGui::CollapsingHeader("Animations"))
      {
        ImGui::Indent();
        for (auto& animation : model->getAnimations())
        {
          if (ImGui::CollapsingHeader(animation.getName().c_str()))
          {
            ImGui::Indent();
            ImGui::Text("Duration: %f", animation.getDuration());
            ImGui::Text("Ticks per second: %f", animation.getTPS());
            ImGui::Text("Total number of nodes: %d", animation.getAniNodes().size());

            ImGui::Separator();
            ImGui::Text("Animation Nodes");
            ImGui::Separator();

            if (this->selectedAniNode)
            {
              if (ImGui::BeginCombo("##combo", this->selectedAniNode->name.c_str()))
              {
                for (auto& aniNode : animation.getAniNodes())
                {
                  bool isSelected = (&aniNode.second == this->selectedAniNode);

                  if (ImGui::Selectable(aniNode.first.c_str(), isSelected))
                    this->selectedAniNode = (&aniNode.second);

                  if (isSelected)
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
              }

              if (ImGui::TreeNode(("Translations##" + this->selectedAniNode->name).c_str()))
              {
                ImGui::Indent();
                for (auto& keyTranslations : this->selectedAniNode->keyTranslations)
                {
                  ImGui::Text("Timestamp: %f, Value: (%f, %f, %f)", keyTranslations.first,
                              keyTranslations.second.x, keyTranslations.second.y,
                              keyTranslations.second.z);
                }
                ImGui::Unindent();
                ImGui::TreePop();
              }
              if (ImGui::TreeNode(("Rotations##" + this->selectedAniNode->name).c_str()))
              {
                ImGui::Indent();
                for (auto& keyRotations : this->selectedAniNode->keyRotations)
                {
                  ImGui::Text("Timestamp: %f, Value: (%f, %f, %f, %f)", keyRotations.first,
                              keyRotations.second.x, keyRotations.second.y,
                              keyRotations.second.z, keyRotations.second.w);
                }
                ImGui::Unindent();
                ImGui::TreePop();
              }
              if (ImGui::TreeNode(("Scales##" + this->selectedAniNode->name).c_str()))
              {
                ImGui::Indent();
                for (auto& keyScales : this->selectedAniNode->keyScales)
                {
                  ImGui::Text("Timestamp: %f, Value: (%f, %f, %f)", keyScales.first,
                              keyScales.second.x, keyScales.second.y,
                              keyScales.second.z);
                }
                ImGui::Unindent();
                ImGui::TreePop();
              }
            }
            else
            {
              if (ImGui::BeginCombo("##combo", ""))
              {
                for (auto& aniNode : animation.getAniNodes())
                {
                  bool isSelected = (&aniNode.second == this->selectedAniNode);

                  if (ImGui::Selectable(aniNode.first.c_str(), isSelected))
                    this->selectedAniNode = (&aniNode.second);

                  if (isSelected)
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
              }
            }
            ImGui::Unindent();
          }
        }
        ImGui::Unindent();
      }
      if (ImGui::CollapsingHeader("Submeshes Properties"))
      {
        auto& submeshes = model->getSubmeshes();

        ImGui::Indent();
        ImGui::Text("Submeshes");
        ImGui::Separator();
        if (this->selectedSubMesh)
        {
          if (ImGui::BeginCombo("##combo", this->selectedSubMesh->getName().c_str()))
          {
            for (auto& submesh : submeshes)
            {
              bool isSelected = (&submesh == this->selectedSubMesh);

              if (ImGui::Selectable(submesh.getName().c_str(), isSelected))
                this->selectedSubMesh = (&submesh);

              if (isSelected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }

          auto material = rComponent.materials.getMaterial(this->selectedSubMesh->getName());

          ImGui::Text("");
          ImGui::Text("Submesh Properties");
          ImGui::Separator();
          ImGui::Text("Submesh name: %s", this->selectedSubMesh->getName().c_str());
          ImGui::Text("Material name: %s", rComponent.materials.getMaterialHandle(this->selectedSubMesh->getName()).c_str());
          if (material->getFilepath() != "")
            ImGui::Text("Material Path: %s", material->getFilepath().c_str());

          if (ImGui::Button(("Open Material Selector##" + std::to_string((unsigned long) this->selectedSubMesh)).c_str()))
          {
            showMaterialWindow = true;
            submeshHandle = this->selectedSubMesh->getName();
          }

          // Actual material properties.
          if (ImGui::CollapsingHeader(("Material Properties##" + std::to_string((unsigned long) this->selectedSubMesh)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
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
              submeshHandle = this->selectedSubMesh->getName();
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
              submeshHandle = this->selectedSubMesh->getName();
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
              submeshHandle = this->selectedSubMesh->getName();
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
              submeshHandle = this->selectedSubMesh->getName();
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
              submeshHandle = this->selectedSubMesh->getName();
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
              submeshHandle = this->selectedSubMesh->getName();
            }
            this->DNDTextureTarget(material, "normalMap");
            ImGui::PopID();
          }

          ImGui::Unindent();
        }
        else
        {
          if (ImGui::BeginCombo("##combo", ""))
          {
            for (auto& submesh : submeshes)
            {
              bool isSelected = (&submesh == this->selectedSubMesh);

              if (ImGui::Selectable(submesh.getName().c_str(), isSelected))
                this->selectedSubMesh = (&submesh);

              if (isSelected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }
        }
        //this->DNDMaterialTarget(submesh.getName());


      }
    }
    ImGui::End();

    if (showTexWindow)
      this->drawTextureWindow(selectedType, submeshHandle, showTexWindow);
    if (showMaterialWindow)
      this->drawMaterialWindow(submeshHandle, showMaterialWindow);
  }

  void
  ModelWindow::onUpdate(float dt, Shared<Scene> activeScene)
  { }

  void
  ModelWindow::onEvent(Event &event)
  {
    switch(event.getType())
    {
      case EventType::EntitySwapEvent:
      {
        auto entSwapEvent = *(static_cast<EntitySwapEvent*>(&event));

        this->selectedNode = nullptr;
        this->selectedAniNode = nullptr;
        this->selectedSubMesh = nullptr;

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

        switch (this->fileLoadTargets)
        {
          case FileLoadTargets::TargetTexture:
          {
            AsyncLoading::loadImageAsync(path);
            this->selectedMatTex.first->attachSampler2D(this->selectedMatTex.second, name);

            this->selectedMatTex = std::make_pair(nullptr, "");
            this->fileLoadTargets = FileLoadTargets::TargetNone;
            break;
          }
          case FileLoadTargets::TargetMaterial:
          {
            auto materialAssets = AssetManager<Material>::getManager();

            AssetHandle handle;
            if (YAMLSerialization::deserializeMaterial(path, handle))
              materialAssets->getAsset(handle)->getFilepath() = path;

            this->fileLoadTargets = FileLoadTargets::TargetNone;
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
  ModelWindow::drawTextureWindow(const std::string &type, const std::string &submesh,
                                    bool &isOpen)
  {
    if (!this->selectedEntity)
      return;

    auto textureCache = AssetManager<Texture2D>::getManager();

    Material* material = this->selectedEntity.getComponent<RenderableComponent>()
                                             .materials.getMaterial(submesh);

    char searchBuffer[256];
    memset(searchBuffer, 0, sizeof(searchBuffer));
    std::strncpy(searchBuffer, this->searched.c_str(), sizeof(searchBuffer));

    ImGui::Begin("Select Texture", &isOpen);

    float cellSize = 64.0f + 16.0f;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int numColumns = (int) (panelWidth / cellSize);
    numColumns = numColumns < 1 ? 1 : numColumns;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
    if (ImGui::Button(ICON_FA_FOLDER_OPEN))
    {
      EventDispatcher* dispatcher = EventDispatcher::getInstance();
      dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                   ".jpg,.tga,.png"));

      this->selectedMatTex = std::make_pair(material, type);
      this->fileLoadTargets = FileLoadTargets::TargetTexture;

      isOpen = false;
    }

    ImGui::SameLine();
    ImGui::Button(ICON_FA_SEARCH);

    ImGui::SameLine();
    if (ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer)))
      this->searched = std::string(searchBuffer);
    ImGui::PopStyleVar();

    ImGui::Columns(numColumns, 0, false);
    for (auto& handle : textureCache->getStorage())
    {
      if (this->searched != "" && handle.find(this->searched) == std::string::npos)
        continue;

      auto texture = textureCache->getAsset(handle);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::ImageButton((ImTextureID) (unsigned long) texture->getID(),
                   ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::PopStyleColor();

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        material->attachSampler2D(type, handle);
        isOpen = false;
        this->selectedHandle = "";
      }
      ImGui::TextWrapped(handle.c_str());

      ImGui::NextColumn();
    }
    ImGui::End();

    if (!isOpen)
      this->selectedHandle = "";
  }

  void
  ModelWindow::drawMaterialWindow(const std::string &submesh, bool &isOpen)
  {
    static bool showNewMaterialWindow = false;

    auto materialAssets = AssetManager<Material>::getManager();

    auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();

    char searchBuffer[256];
    memset(searchBuffer, 0, sizeof(searchBuffer));
    std::strncpy(searchBuffer, this->searched.c_str(), sizeof(searchBuffer));

    ImGui::Begin("Select Material", &isOpen);

    float cellSize = 64.0f + 16.0f;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int numColumns = (int) (panelWidth / cellSize);
    numColumns = numColumns < 1 ? 1 : numColumns;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
    if (ImGui::Button(ICON_FA_FILE))
      showNewMaterialWindow = true;

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_OPEN))
    {
      EventDispatcher* dispatcher = EventDispatcher::getInstance();
      dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                   ".smtl"));
      this->fileLoadTargets = FileLoadTargets::TargetMaterial;
    }

    ImGui::SameLine();
    ImGui::Button(ICON_FA_SEARCH);

    ImGui::SameLine();
    if (ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer)))
      this->searched = std::string(searchBuffer);
    ImGui::PopStyleVar();

    ImGui::Columns(numColumns, 0, false);
    for (auto& handle : materialAssets->getStorage())
    {
      if (this->searched != "" && handle.find(this->searched) == std::string::npos)
        continue;

      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::ImageButton((ImTextureID) (unsigned long)
                         materialAssets->getAsset(handle)->getSampler2D("albedoMap")->getID(),
                         ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::PopStyleColor();

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        rComponent.materials.swapMaterial(submesh, handle);
        isOpen = false;
      }
      ImGui::TextWrapped(handle.c_str());

      ImGui::NextColumn();
    }

    ImGui::End();

    if (showNewMaterialWindow)
      this->drawNewMaterialWindow(submesh, showNewMaterialWindow);
  }

  void
  ModelWindow::drawNewMaterialWindow(const std::string &submesh, bool &isOpen)
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

      materialAssets->attachAsset(this->newMaterialName, new Material());

      this->newMaterialName = "";
      isOpen = false;
    }
    ImGui::End();
  }

  void
  ModelWindow::DNDTextureTarget(Material* material, const std::string &selectedType)
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
  ModelWindow::DNDMaterialTarget(const std::string &subMesh)
  {
    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
        this->loadDNDMaterial((char*) payload->Data, subMesh);

      ImGui::EndDragDropTarget();
    }
  }

  void
  ModelWindow::loadDNDTextureAsset(const std::string &filepath)
  {
    std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
    std::string filetype = filename.substr(filename.find_last_of('.'));

    if (filetype == ".jpg" || filetype == ".tga" || filetype == ".png")
    {
      AsyncLoading::loadImageAsync(filepath);
      this->selectedMatTex.first->attachSampler2D(this->selectedMatTex.second, filename);
    }

    this->selectedMatTex = std::make_pair(nullptr, "");
  }

  void
  ModelWindow::loadDNDMaterial(const std::string &filepath, const std::string &subMesh)
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
