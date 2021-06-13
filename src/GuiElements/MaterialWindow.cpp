#include "GuiElements/MaterialWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  MaterialWindow::MaterialWindow()
  { }

  MaterialWindow::~MaterialWindow()
  { }

  void
  MaterialWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    static bool showTexWindow = false;
    static std::string selectedType;
    std::string selectedMeshName;
    Shared<Mesh> submesh;

    if (this->selectedEntity)
    {
      if (this->selectedEntity.hasComponent<RenderableComponent>())
      {
        ImGui::Begin("Materials", &isOpen);

        auto& rComponent = this->selectedEntity.getComponent<RenderableComponent>();

        for (auto& pair : rComponent.model->getSubmeshes())
        {
          if (ImGui::CollapsingHeader((pair.first + "##" + std::to_string((unsigned long) pair.second.get())).c_str()))
          {
            selectedMeshName = pair.first;
            submesh = pair.second;

            auto material = rComponent.materials.getMaterial(pair.second);
            auto& uAlbedo = material->getVec3("uAlbedo");
            auto& uMetallic = material->getFloat("uMetallic");
            auto& uRoughness = material->getFloat("uRoughness");
            auto& uAO = material->getFloat("uAO");

            // Draw all the associated texture maps for the entity.
            ImGui::Text("Albedo Map");
            ImGui::PushID("Albedo Button");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("albedoMap")->getID(),
                                   ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "albedoMap";
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::ColorEdit3("##Albedo", &uAlbedo.r);

            ImGui::Text("Metallic Map");
            ImGui::PushID("Metallic Button");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("metallicMap")->getID(),
                                   ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "metallicMap";
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::SliderFloat("##Metallic", &uMetallic, 0.0f, 1.0f);

            ImGui::Text("Roughness Map");
            ImGui::PushID("Roughness Button");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("roughnessMap")->getID(),
                                   ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "roughnessMap";
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::SliderFloat("##Roughness", &uRoughness, 0.0f, 1.0f);

            ImGui::Text("Ambient Occlusion Map");
            ImGui::PushID("Ambient Button");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("aOcclusionMap")->getID(),
                                   ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "aOcclusionMap";
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::SliderFloat("##AO", &uAO, 0.0f, 1.0f);

            ImGui::Text("Normal Map");
            ImGui::PushID("Normal Button");
            if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getTexture2D("normalMap")->getID(),
                                   ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
            {
              showTexWindow = true;
              selectedType = "normalMap";
            }
            ImGui::PopID();
          }
        }

        ImGui::End();
      }
    }
    if (showTexWindow)
      this->drawTextureWindow(selectedType, submesh, showTexWindow);
  }

  void
  MaterialWindow::onUpdate(float dt)
  {

  }

  void
  MaterialWindow::onEvent(Event &event)
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
          case FileLoadTargets::TargetTexture:
          {
            Texture2D* tex = Texture2D::loadTexture2D(path);
            this->selectedMatTex.first->attachTexture2D(tex, this->selectedMatTex.second);

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
  MaterialWindow::drawTextureWindow(const std::string &type, Shared<Mesh> submesh,
                                    bool &isOpen)
  {
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

    if (!isOpen)
      this->selectedString = "";
  }
}
