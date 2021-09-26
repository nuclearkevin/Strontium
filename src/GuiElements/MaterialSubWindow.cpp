#include "GuiElements/MaterialSubWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Scenes/Components.h"
#include "GuiElements/Styles.h"
#include "Serialization/YamlSerialization.h"
#include "Utils/AsyncAssetLoading.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Strontium
{
  // UI helper functions.
  //----------------------------------------------------------------------------
  void
  loadDNDTexture(Material* target, const std::string &mapType)
  {
    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
      {
        std::string filepath = (char*) payload->Data;
        std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
        std::string filetype = filename.substr(filename.find_last_of('.'));

        if (filetype == ".jpg" || filetype == ".tga" || filetype == ".png")
        {
          AsyncLoading::loadImageAsync(filepath);
          target->attachSampler2D(mapType, filename);
        }
      }

      ImGui::EndDragDropTarget();
    }
  }

  void
  materialMapSliderFloat(const std::string &name, const std::string &samplerName,
                         Material* material, float &property, std::string &selectedMap,
                         float min = 0.0f, float max = 1.0f)
  {
    ImGui::Text(name.c_str());
    ImGui::PushID(name.c_str());
    if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D(samplerName)->getID(),
                           ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
    {
      selectedMap = samplerName;
    }
    loadDNDTexture(material, samplerName);
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::SliderFloat(std::string(std::string("##") + samplerName).c_str(), &property, min, max);
  }

  void
  drawTextureCacheWindow(Material* material, std::string &samplerName, bool &isOpen, FileLoadTargets &fileLoadTargets)
  {
    if (samplerName == "" || !isOpen)
      return;

    auto textureCache = AssetManager<Texture2D>::getManager();

    static std::string searched = "";

    char searchBuffer[256];
    memset(searchBuffer, 0, sizeof(searchBuffer));
    std::strncpy(searchBuffer, searched.c_str(), sizeof(searchBuffer));

    ImGui::Begin("Stored Textures", &isOpen);

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

      fileLoadTargets = FileLoadTargets::TargetTexture;
    }

    ImGui::SameLine();
    ImGui::Button(ICON_FA_SEARCH);

    ImGui::SameLine();
    if (ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer)))
      searched = std::string(searchBuffer);
    ImGui::PopStyleVar();

    ImGui::Columns(numColumns, 0, false);
    for (auto& handle : textureCache->getStorage())
    {
      if (searched != "" && handle.find(searched) == std::string::npos)
        continue;

      auto texture = textureCache->getAsset(handle);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::ImageButton((ImTextureID) (unsigned long) texture->getID(),
                   ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::PopStyleColor();

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
      {
        material->attachSampler2D(samplerName, handle);
        isOpen = false;
        samplerName = "";
      }
      ImGui::TextWrapped(handle.c_str());

      ImGui::NextColumn();
    }
    ImGui::End();

    if (!isOpen)
      samplerName = "";
  }
  //----------------------------------------------------------------------------

  MaterialSubWindow::MaterialSubWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer, false)
    , fileLoadTargets(FileLoadTargets::TargetNone)
  { }

  MaterialSubWindow::~MaterialSubWindow()
  { }

  void
  MaterialSubWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    if (this->selectedMaterial == "")
      return;

    auto materialAssets = AssetManager<Material>::getManager();
    auto material = materialAssets->getAsset(this->selectedMaterial);

    if (!material)
      return;

    static std::string selectedSampler = "";
    static bool showTexWindow = false;

    char nameBuffer[256];
    char pathBuffer[256];
    memset(nameBuffer, 0, sizeof(nameBuffer));
    memset(pathBuffer, 0, sizeof(pathBuffer));

    std::strncpy(nameBuffer, this->selectedMaterial.c_str(), sizeof(nameBuffer));
    if (material->getFilepath() != "")
      std::strncpy(pathBuffer, material->getFilepath().c_str(), sizeof(pathBuffer));

    float fontSize = ImGui::GetFontSize();
    ImGui::Begin("Material Editor", &isOpen);

    ImGui::Text("Material name:");
    ImGui::SameLine();
    ImGui::InputText("##MaterialName", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_ReadOnly);

    if (material->getFilepath() != "")
    {
      ImGui::Text("Material Path:");
      ImGui::SameLine();
      ImGui::InputText("##MaterialPath", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_ReadOnly);
    }

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
      selectedSampler = "albedoMap";
    }
    loadDNDTexture(material, "albedoMap");
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

    materialMapSliderFloat("Metallic Map", "metallicMap", material, uMetallic, selectedSampler);
    materialMapSliderFloat("Roughness Map", "roughnessMap", material, uRoughness, selectedSampler);
    materialMapSliderFloat("Ambient Occlusion Map", "aOcclusionMap", material, uAO, selectedSampler);
    materialMapSliderFloat("Specular F0 Map", "specF0Map", material, uF0, selectedSampler);

    ImGui::Text("Normal Map");
    ImGui::PushID("Normal Button");
    if (ImGui::ImageButton((ImTextureID) (unsigned long) material->getSampler2D("normalMap")->getID(),
                           ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
    {
      selectedSampler = "normalMap";
    }
    loadDNDTexture(material, "normalMap");
    ImGui::PopID();
    ImGui::End();

    if (selectedSampler != "")
      showTexWindow = true;

    drawTextureCacheWindow(material, selectedSampler, showTexWindow, this->fileLoadTargets);
  }

  void
  MaterialSubWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  MaterialSubWindow::onEvent(Event &event)
  {
    switch(event.getType())
    {
      case EventType::LoadFileEvent:
      {
        auto loadEvent = *(static_cast<LoadFileEvent*>(&event));
        auto& path = loadEvent.getAbsPath();
        auto& name = loadEvent.getFileName();

        switch(this->fileLoadTargets)
        {
          case FileLoadTargets::TargetTexture:
          {
            AsyncLoading::loadImageAsync(path);
            break;
          }
          default: break;
        }
        this->fileLoadTargets = FileLoadTargets::TargetNone;
        break;
      }
      default: break;
    }
  }
}
