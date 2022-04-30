#include "GuiElements/MaterialSubWindow.h"

// Project includes.
#include "Core/Application.h"

#include "Assets/AssetManager.h"
#include "Assets/Image2DAsset.h"
#include "Assets/MaterialAsset.h"

#include "Serialization/YamlSerialization.h"
#include "Utils/AsyncAssetLoading.h"

#include "Scenes/Components.h"

#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// STL includes.
#include <filesystem>

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
        std::filesystem::path fsPath(filepath);

        std::string filename = fsPath.filename().string();
        std::string filetype = fsPath.extension().string();

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
    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(material->getSampler2D(samplerName)->getID()),
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

    auto& assetCache = Application::getInstance()->getAssetCache();

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
    for (auto& [handle, asset] : assetCache.getPool<Image2DAsset>())
    {
      if (searched != "" && handle.find(searched) == std::string::npos)
        continue;

      auto texture = static_cast<Image2DAsset*>(asset.get())->getTexture();
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::ImageButton(reinterpret_cast<ImTextureID>(texture->getID()),
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

    auto& assetCache = Application::getInstance()->getAssetCache();
    auto materialAsset = assetCache.get<MaterialAsset>(this->selectedMaterial);
    auto material = materialAsset->getMaterial();

    if (!material)
      return;

    static std::string selectedSampler = "";
    static bool showTexWindow = false;

    char nameBuffer[256];
    char pathBuffer[256];
    memset(nameBuffer, 0, sizeof(nameBuffer));
    memset(pathBuffer, 0, sizeof(pathBuffer));

    std::strncpy(nameBuffer, this->selectedMaterial.c_str(), sizeof(nameBuffer));
    if (!materialAsset->getPath().empty())
      std::strncpy(pathBuffer, materialAsset->getPath().string().c_str(), sizeof(pathBuffer));

    float fontSize = ImGui::GetFontSize();
    ImGui::Begin("Material Editor", &isOpen);

    ImGui::Text("Material name:");
    ImGui::SameLine();
    ImGui::InputText("##MaterialName", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_ReadOnly);

    if (!materialAsset->getPath().empty())
    {
      ImGui::Text("Material Path:");
      ImGui::SameLine();
      ImGui::InputText("##MaterialPath", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_ReadOnly);
    }

    auto uAlbedo = material->getvec3("uAlbedo");
    auto uMetallic = material->getfloat("uMetallic");
    auto uRoughness = material->getfloat("uRoughness");
    auto uAO = material->getfloat("uAO");
    auto uEmiss = material->getfloat("uEmiss");
    auto reflectance = material->getfloat("uReflectance");

    // Draw all the associated texture maps for the entity.
    ImGui::Text("Albedo Map");
    ImGui::PushID("Albedo Button");
    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(material->getSampler2D("albedoMap")->getID()),
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

    materialMapSliderFloat("Emission Map", "emissionMap", material, uEmiss, selectedSampler, 0.0f, 100.0f);
    materialMapSliderFloat("Metallic Map", "metallicMap", material, uMetallic, selectedSampler);
    materialMapSliderFloat("Roughness Map", "roughnessMap", material, uRoughness, selectedSampler);
    materialMapSliderFloat("Ambient Occlusion Map", "aOcclusionMap", material, uAO, selectedSampler);
    materialMapSliderFloat("Reflectance Map", "specF0Map", material, reflectance, selectedSampler);

    material->set(uAlbedo, "uAlbedo");
    material->set(uMetallic, "uMetallic");
    material->set(uRoughness, "uRoughness");
    material->set(uAO, "uAO");
    material->set(uEmiss, "uEmiss");

    material->set(reflectance, "uReflectance");

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
