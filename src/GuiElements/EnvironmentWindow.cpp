#include "GuiElements/EnvironmentWindow.h"

namespace SciRenderer
{
  EnvironmentWindow::EnvironmentWindow()
    : drawIrrad(false)
    , drawFilter(false)
    , mapRes(512)
  { }

  EnvironmentWindow::~EnvironmentWindow()
  { }

  void
  EnvironmentWindow::onAttach()
  {
    // Initialize the PBR skybox.
  	this->skybox = new EnvironmentMap("./res/shaders/pbr/pbrSkybox.vs",
  													          "./res/shaders/pbr/pbrSkybox.fs",
  															      "./res/models/cube.obj");
  	this->skybox->loadEquirectangularMap("./res/textures/hdr_environments/checkers.hdr");
  	this->skybox->equiToCubeMap();
  	this->skybox->precomputeIrradiance();
  	this->skybox->precomputeSpecular();
  }

  void
  EnvironmentWindow::onDetach()
  {
    delete this->skybox;
  }

  void
  EnvironmentWindow::onImGuiRender()
  {
    bool openEnvironment = false;

    ImGui::Begin("Environment", nullptr);
    if (!this->skybox->hasSkybox())
    {
      if (ImGui::Button("Load Equirectangular Map"))
      {
        this->skybox->unloadEnvironment();
        openEnvironment = true;
      }
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("Load Equirectangular Map");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }

    ImGui::SameLine();
    if (this->skybox->hasSkybox())
    {
      if (ImGui::Button("Clear Environment Map"))
      {
        this->skybox->unloadEnvironment();
        this->drawIrrad = false;
        this->drawFilter = false;
        this->skybox->setDrawingType(MapType::Skybox);
      }
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button("Clear Environment Map");
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }

    if (this->skybox->hasEqrMap() && !this->skybox->hasSkybox())
    {
      ImGui::Text("Preview:");
      ImGui::Image((ImTextureID) this->skybox->getTexID(MapType::Equirectangular),
                   ImVec2(360, 180), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::SliderInt("Environment Resolution", &this->mapRes, 512, 2048);
      if (ImGui::Button("Generate Skybox"))
      {
        this->skybox->equiToCubeMap(true, this->mapRes, this->mapRes);
      }
    }

    if (this->skybox->hasSkybox())
    {
      ImGui::SliderFloat("Gamma", &this->skybox->getGamma(), 1.0f, 5.0f);
      ImGui::SliderFloat("Exposure", &this->skybox->getExposure(), 1.0f, 5.0f);

      if (!this->skybox->hasIrradiance())
      {
        if (ImGui::Button("Generate Irradiance Map"))
        {
          this->skybox->precomputeIrradiance(256, 256, true);
        }
      }
      else
      {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Generate Irradiance Map");
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();

        ImGui::Checkbox("Draw Irradiance Map", &this->drawIrrad);
        if (this->drawIrrad && !this->skybox->drawingFilter())
        {
          this->skybox->setDrawingType(MapType::Irradiance);
        }
        else if (!this->drawIrrad && !skybox->drawingFilter())
        {
          this->skybox->setDrawingType(MapType::Skybox);
        }
        else
        {
          this->drawIrrad = false;
        }
      }

      if (!this->skybox->hasPrefilter())
      {
        if (ImGui::Button("Generate BRDF Specular Map"))
        {
          this->skybox->precomputeSpecular(this->mapRes, this->mapRes);
        }
      }
      else
      {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Generate BRDF Specular Map");
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();

        ImGui::Checkbox("Draw Pre-Filter Map", &this->drawFilter);
        if (this->drawFilter && !this->skybox->drawingIrrad())
        {
          this->skybox->setDrawingType(MapType::Prefilter);
          ImGui::SliderFloat("Roughness", &this->skybox->getRoughness(), 0.0f, 1.0f);
        }
        else if (!this->drawFilter && !skybox->drawingIrrad())
        {
          this->skybox->setDrawingType(MapType::Skybox);
        }
        else
        {
          this->drawFilter = false;
        }
      }

      if (skybox->hasIntegration())
      {
        ImGui::Text("BRDF Lookup Texture:");
        ImGui::Image((ImTextureID) this->skybox->getTexID(MapType::Integration),
                     ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
      }
    }

    ImGui::End();

    // Open a file browser to load .hdr files.
    if (openEnvironment)
      ImGui::OpenPopup("Load Equirectangular Map");

    if (this->fileHandler.showFileDialog("Load Equirectangular Map",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".hdr"))
    {
      this->skybox->loadEquirectangularMap(this->fileHandler.selected_path);
    }
  }

  void
  EnvironmentWindow::onUpdate(float dt)
  {

  }

  void
  EnvironmentWindow::onEvent(Event &event)
  {

  }
}
