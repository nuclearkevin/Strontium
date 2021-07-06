#include "GuiElements/RendererWindow.h"

// Project includes.
#include "Graphics/Renderer.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "GuiElements/Styles.h"

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
    auto storage = Renderer3D::getStorage();
    auto state = Renderer3D::getState();
    auto stats = Renderer3D::getStats();

    ImGui::Begin("Renderer Settings", &isOpen);

    ImGui::Text("Drawcalls: %u", stats->drawCalls);
    ImGui::Text("Total vertices: %u", stats->numVertices);
    ImGui::Text("Total triangles: %u", stats->numTriangles);
    ImGui::Text("Total lights: D-%u P-%u S-%u", stats->numDirLights,
                stats->numPointLights, stats->numSpotLights);

    if (ImGui::CollapsingHeader("Shadows"))
    {
      Styles::drawFloatControl("Cascade Lambda", 0.5f, state->cascadeLambda, 0.0f, 0.01f, 0.5f, 1.0f);
      Styles::drawFloatControl("Shadow Softness", 0.0f, state->sampleRadius, 0.0f, 0.01f, 0.0f, 5.0f);

      int shadowWidth = state->cascadeSize;
      if (ImGui::InputInt("Shadowmap Size", &shadowWidth))
      {
        shadowWidth = shadowWidth > 4096 ? 4096 : shadowWidth;
        shadowWidth = shadowWidth < 512 ? 512 : shadowWidth;

        // Compute the next or previous power of 2 and set the int to that.
        if (shadowWidth - (int) state->cascadeSize < 0)
        {
          shadowWidth = std::pow(2, std::floor(std::log2(shadowWidth)));
        }
        else if (shadowWidth - (int) state->cascadeSize > 0)
        {
          shadowWidth = std::pow(2, std::floor(std::log2(shadowWidth)) + 1);
        }

        shadowWidth = std::pow(2, std::floor(std::log2(shadowWidth)));
        state->cascadeSize = shadowWidth;

        for (unsigned int i = 0; i < NUM_CASCADES; i++)
          storage->shadowBuffer[i].resize(state->cascadeSize, state->cascadeSize);
      }

      static bool showMaps = false;
      ImGui::Checkbox("Show shadow maps", &showMaps);

      if (showMaps)
      {
        static int cascadeIndex = 0;
        ImGui::SliderInt("Cascade Index", &cascadeIndex, 0, 3);
        ImGui::Text("Depth:");
        ImGui::Image((ImTextureID) (unsigned long) storage->shadowBuffer[(unsigned int) cascadeIndex].getAttachID(FBOTargetParam::Depth),
                     ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Text("Moments");
        ImGui::Image((ImTextureID) (unsigned long) storage->shadowBuffer[(unsigned int) cascadeIndex].getAttachID(FBOTargetParam::Colour0),
                     ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      }
    }

    if (ImGui::CollapsingHeader("Render Passes"))
    {
      auto bufferSize = storage->geometryPass.getSize();
      GLfloat ratio = bufferSize.x / bufferSize.y;

      ImGui::Separator();
      ImGui::Text("Lighting:");
      ImGui::Separator();
      ImGui::Image((ImTextureID) (unsigned long) storage->lightingPass.getAttachID(FBOTargetParam::Colour0),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Separator();
      ImGui::Text("GBuffer:");
      ImGui::Separator();
      ImGui::Text("Positions:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour0),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Normals:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour1),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Albedo:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour2),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Materials:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour3),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Entity Mask:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Colour4),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Depth:");
      ImGui::Image((ImTextureID) (unsigned long) storage->geometryPass.getAttachID(FBOTargetParam::Depth),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
    }

    if (ImGui::CollapsingHeader("Environment Map"))
    {
      int skyboxWidth = state->skyboxWidth;
      int irradianceWidth = state->irradianceWidth;
      int prefilterWidth = state->prefilterWidth;
      int prefilterSamples = state->prefilterSamples;

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
