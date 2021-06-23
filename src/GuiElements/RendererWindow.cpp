#include "GuiElements/RendererWindow.h"

// Project includes.
#include "Graphics/Renderer.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  RendererWindow::RendererWindow()
    : GuiWindow()
  {

  }

  RendererWindow::~RendererWindow()
  {

  }

  void
  RendererWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    auto state = Renderer3D::getState();
    int skyboxWidth = state->skyboxWidth;
    int irradianceWidth = state->irradianceWidth;
    int prefilterWidth = state->prefilterWidth;
    int prefilterSamples = state->prefilterSamples;

    auto stats = Renderer3D::getStats();

    ImGui::Begin("Renderer Settings", &isOpen);

    ImGui::Text("Drawcalls: %u", stats->drawCalls);
    ImGui::Text("Total vertices: %u", stats->numVertices);
    ImGui::Text("Total triangles: %u", stats->numTriangles);

    if (ImGui::CollapsingHeader("Geometry Pass"))
    {
      auto storage = Renderer3D::getStorage();
      ImGui::Text("Positions:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour0),
                   ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Normals:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour1),
                   ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Albedo:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour2),
                   ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Materials:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour3),
                   ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
    }

    if (ImGui::CollapsingHeader("Lighting Pass"))
    {
      auto storage = Renderer3D::getStorage();
      ImGui::Image((ImTextureID) (unsigned long) storage->lightingPass.getAttachID(FBOTargetParam::Colour0),
                   ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
    }

    if (ImGui::CollapsingHeader("Environment Map"))
    {
      if (ImGui::Button("Recompute Environment Map"))
      {
        auto ambient = Renderer3D::getStorage()->currentEnvironment.get();

        ambient->unloadComputedMaps();
        ambient->equiToCubeMap(true, state->skyboxWidth, state->skyboxWidth);
        ambient->precomputeIrradiance(state->irradianceWidth, state->irradianceWidth, true);
        ambient->precomputeSpecular(state->prefilterWidth, state->prefilterWidth, true);
      }

      if (ImGui::InputInt("Skybox Size", &skyboxWidth))
      {
        skyboxWidth = skyboxWidth > 2048 ? 2048 : skyboxWidth;
        skyboxWidth = skyboxWidth < 512 ? 512 : skyboxWidth;

        // Compute the next or previous power of 2 and set the int to that.
        if (skyboxWidth - (int) state->skyboxWidth < 0)
        {
          skyboxWidth = std::pow(2, std::floor(std::log2(skyboxWidth)));
        }
        else if (skyboxWidth - (int) state->skyboxWidth > 0)
        {
          skyboxWidth = std::pow(2, std::floor(std::log2(skyboxWidth)) + 1);
        }

        skyboxWidth = std::pow(2, std::floor(std::log2(skyboxWidth)));
        state->skyboxWidth = skyboxWidth;
      }

      if (ImGui::InputInt("Irradiance Map Quality", &irradianceWidth))
      {
        irradianceWidth = irradianceWidth > 512 ? 512 : irradianceWidth;
        irradianceWidth = irradianceWidth < 64 ? 64 : irradianceWidth;

        // Compute the next or previous power of 2 and set the int to that.
        if (irradianceWidth - (int) state->irradianceWidth < 0)
        {
          irradianceWidth = std::pow(2, std::floor(std::log2(irradianceWidth)));
        }
        else if (irradianceWidth - (int) state->irradianceWidth > 0)
        {
          irradianceWidth = std::pow(2, std::floor(std::log2(irradianceWidth)) + 1);
        }

        irradianceWidth = std::pow(2, std::floor(std::log2(irradianceWidth)));
        state->irradianceWidth = irradianceWidth;
      }

      if (ImGui::InputInt("IBL Prefilter Quality", &prefilterWidth))
      {
        prefilterWidth = prefilterWidth > 2048 ? 2048 : prefilterWidth;
        prefilterWidth = prefilterWidth < 512 ? 512 : prefilterWidth;

        // Compute the next or previous power of 2 and set the int to that.
        if (prefilterWidth - (int) state->prefilterWidth < 0)
        {
          prefilterWidth = std::pow(2, std::floor(std::log2(prefilterWidth)));
        }
        else if (prefilterWidth - (int) state->prefilterWidth > 0)
        {
          prefilterWidth = std::pow(2, std::floor(std::log2(prefilterWidth)) + 1);
        }

        prefilterWidth = std::pow(2, std::floor(std::log2(prefilterWidth)));
        state->prefilterWidth = prefilterWidth;
      }

      if (ImGui::InputInt("IBL Prefilter Samples", &prefilterSamples))
      {
        prefilterSamples = prefilterSamples > 2048 ? 2048 : prefilterSamples;
        prefilterSamples = prefilterSamples < 512 ? 512 : prefilterSamples;

        // Compute the next or previous power of 2 and set the int to that.
        if (prefilterSamples - (int) state->prefilterSamples < 0)
        {
          prefilterSamples = std::pow(2, std::floor(std::log2(prefilterSamples)));
        }
        else if (prefilterSamples - (int) state->prefilterSamples > 0)
        {
          prefilterSamples = std::pow(2, std::floor(std::log2(prefilterSamples)) + 1);
        }

        prefilterSamples = std::pow(2, std::floor(std::log2(prefilterSamples)));
        state->prefilterSamples = prefilterSamples;
      }
    }

    ImGui::End();
  }

  void
  RendererWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  RendererWindow::onEvent(Event &event)
  {

  }
}
