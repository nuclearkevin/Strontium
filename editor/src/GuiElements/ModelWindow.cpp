#include "GuiElements/ModelWindow.h"

// Project includes.
#include "Core/Application.h"
#include "Assets/AssetManager.h"
#include "Assets/ModelAsset.h"

#include "Serialization/YamlSerialization.h"
#include "Utils/AsyncAssetLoading.h"

#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// Other includes.
#include "glm/gtx/string_cast.hpp"

namespace Strontium
{
  ModelWindow::ModelWindow(EditorLayer* parentLayer, bool isOpen)
    : GuiWindow(parentLayer, isOpen)
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
    auto& assetCache = Application::getInstance()->getAssetCache();
    float fontSize = ImGui::GetFontSize();
    static bool showMaterialWindow = false;
    static std::string selectedType, submeshHandle;

    if (!this->selectedEntity)
      return;

    if (!this->selectedEntity.hasComponent<RenderableComponent>())
      return;

    ImGui::Begin("Model Information", &isOpen);

    auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();
    auto model = assetCache.get<ModelAsset>(rComponent.meshName)->getModel();
    if (model)
    {
      ImGui::Text("Model name: %s", rComponent.meshName.c_str());
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
          if (ImGui::BeginCombo("##sceneNodes", this->selectedNode->name.c_str()))
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
          if (ImGui::BeginCombo("##sceneNodes", ""))
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
              if (ImGui::BeginCombo("##aniNodes", this->selectedAniNode->name.c_str()))
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
              if (ImGui::BeginCombo("##aniNodes", ""))
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
          if (ImGui::BeginCombo("##submeshes", this->selectedSubMesh->getName().c_str()))
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

          ImGui::Text("");
          ImGui::Text("Submesh Properties");
          ImGui::Separator();

          ImGui::Unindent();
        }
        else
        {
          if (ImGui::BeginCombo("##submeshes", ""))
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
      }
    }

    ImGui::End();
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
          default: break;
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
          default: break;
        }

        break;
      }
      default: break;
    }
  }
}
