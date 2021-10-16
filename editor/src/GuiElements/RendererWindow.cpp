#include "GuiElements/RendererWindow.h"

// Project includes.
#include "Graphics/Renderer.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "GuiElements/Styles.h"

namespace Strontium
{
  RendererWindow::RendererWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
  { }

  RendererWindow::~RendererWindow()
  { }

  void
  RendererWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    auto storage = Renderer3D::getStorage();
    auto state = Renderer3D::getState();
    auto stats = Renderer3D::getStats();

    ImGui::Begin("Renderer Settings", &isOpen);

    ImGui::Text("Geometry pass frametime: %f ms", stats->geoFrametime);
    ImGui::Text("Shadow pass frametime: %f ms", stats->shadowFrametime);
    ImGui::Text("Lighting pass frametime: %f ms", stats->lightFrametime);
    ImGui::Text("Post-processing pass frametime: %f ms", stats->postFramtime);
    ImGui::Text("");

    ImGui::Text("Drawcalls: %u", stats->drawCalls);
    ImGui::Text("Total vertices: %u", stats->numVertices);
    ImGui::Text("Total triangles: %u", stats->numTriangles);
    ImGui::Text("Total lights: D: %u, P: %u, S: %u", stats->numDirLights,
                stats->numPointLights, stats->numSpotLights);

    ImGui::Checkbox("Frustum Cull", &state->frustumCull);
    ImGui::Checkbox("Enable FXAA", &state->enableFXAA);

    // TODO: Soft shadow quality settings (Hard shadows, Low, medium, high, ultra). 
    // Low is a simple box blur, medium->ultra are gaussian with different number 
    // of taps.Hard shadows are regular shadow maps with zero prefiltering.
    if (ImGui::CollapsingHeader("Shadows"))
    {
      static int cascadeIndex = 0;
      ImGui::SliderInt("Cascade Index", &cascadeIndex, 0, NUM_CASCADES - 1);

      ImGui::DragFloat("Cascade Lambda", &state->cascadeLambda, 0.01f, 0.5f, 1.0f);
      ImGui::DragFloat("Bleed Reduction", &state->bleedReduction, 0.01f, 0.0f, 0.9f);

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

        storage->shadowEffectsBuffer.resize(state->cascadeSize, state->cascadeSize);
      }

      static bool showMaps = false;
      ImGui::Checkbox("Show shadow maps", &showMaps);

      if (showMaps)
      {
        ImGui::Text("Depth");
        ImGui::Image((ImTextureID) (unsigned long) storage->shadowBuffer[(unsigned int) cascadeIndex].getAttachID(FBOTargetParam::Depth),
                     ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Text("Moments");
        ImGui::Image((ImTextureID) (unsigned long) storage->shadowBuffer[(unsigned int) cascadeIndex].getAttachID(FBOTargetParam::Colour0),
                     ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      }
    }

    if (ImGui::CollapsingHeader("Volumetric Lights"))
    {
      auto bufferSize = storage->gBuffer.getSize();
      float ratio = bufferSize.x / bufferSize.y;

      ImGui::Checkbox("Enable Godrays", &state->enableSkyshafts);

      Styles::drawFloatControl("Volumetric Intensity", 1.0f, state->mieScatIntensity.w,
                               0.0f, 0.1f, 0.0f, 100.0f);
      Styles::drawFloatControl("Particle Density", 1.0f, state->mieAbsDensity.w,
                               0.0f, 0.1f, 0.0f, 1000.0f);

      glm::vec3 mieAbs = glm::vec3(state->mieAbsDensity);
      Styles::drawVec3Controls("Mie Absorption", glm::vec3(4.4f),
                               mieAbs,  0.0f, 0.01f,
                               0.0f, 10.0f);
      state->mieAbsDensity.x = mieAbs.x;
      state->mieAbsDensity.y = mieAbs.y;
      state->mieAbsDensity.z = mieAbs.z;

      glm::vec3 mieScat = glm::vec3(state->mieScatIntensity);
      Styles::drawVec3Controls("Mie Scattering", glm::vec3(4.0f),
                               mieScat, 0.0f, 0.01f,
                               0.0f, 10.0f);
      state->mieScatIntensity.x = mieScat.x;
      state->mieScatIntensity.y = mieScat.y;
      state->mieScatIntensity.z = mieScat.z;

      static bool showLightTexture = false;
      ImGui::Checkbox("Show Volumetric Light Texture", &showLightTexture);

      if (showLightTexture)
      {
        ImGui::Text("Sunshaft Texture");
        ImGui::Image((ImTextureID) (unsigned long) storage->downsampleLightshaft.getID(),
                     ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      }
    }

    if (ImGui::CollapsingHeader("Bloom"))
    {
      auto bufferSize = storage->gBuffer.getSize();
      float ratio = bufferSize.x / bufferSize.y;
      static int downMipView = 0;
      static int upMipView = 0;
      static int bufferMipView = 0;

      ImGui::Checkbox("Use Bloom", &state->enableBloom);
      ImGui::DragFloat("Threshold", &state->bloomThreshold, 0.01f, 0.0f, 10.0f);
      ImGui::DragFloat("Knee", &state->bloomKnee, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Radius", &state->bloomRadius, 0.01f, 0.0f, 10.0f);
      ImGui::DragFloat("Intensity", &state->bloomIntensity, 0.01f, 0.0f, 10.0f);
      ImGui::Indent();
      if (ImGui::CollapsingHeader("Debug View##bloom"))
      {
        ImGui::Text("Downsample Image Pyramid (%d, %d)",
                    storage->downscaleBloomTex[downMipView].width,
                    storage->downscaleBloomTex[downMipView].height);
        ImGui::SliderInt("Mip##downSample", &downMipView, 0, MAX_NUM_BLOOM_MIPS - 1);
        ImGui::Image((ImTextureID) (unsigned long) storage->downscaleBloomTex[downMipView].getID(),
                     ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::Text("Upsample Image Pyramid (%d, %d)",
                    storage->upscaleBloomTex[upMipView].width,
                    storage->upscaleBloomTex[upMipView].height);
        ImGui::SliderInt("Mip##upSample", &upMipView, 0, MAX_NUM_BLOOM_MIPS - 1);
        ImGui::Image((ImTextureID) (unsigned long) storage->upscaleBloomTex[upMipView].getID(),
                     ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::Text("Buffer Image Pyramid (%d, %d)",
                    storage->bufferBloomTex[bufferMipView].width,
                    storage->bufferBloomTex[bufferMipView].height);
        ImGui::SliderInt("Mip##buffer", &bufferMipView, 0, MAX_NUM_BLOOM_MIPS - 2);
        ImGui::Image((ImTextureID) (unsigned long) storage->bufferBloomTex[bufferMipView].getID(),
                     ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      }
      ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Tone Mapping"))
    {
      ImGui::DragFloat("Gamma", &state->gamma, 0.01f, 1.0f, 10.0f);

      const char* toneMapOps[] = { "Reinhard", "Enhanced Reinhard", "Reinhard-Jodie", "Uncharted 2", "Fast ACES", "ACES", "None" };

      if (ImGui::BeginCombo("##toneMapCombo", toneMapOps[state->postProcessSettings.x]))
      {
        for (unsigned int i = 0; i < IM_ARRAYSIZE(toneMapOps); i++)
        {
          bool isSelected = (toneMapOps[i] == toneMapOps[state->postProcessSettings.x]);
          if (ImGui::Selectable(toneMapOps[i], isSelected))
            state->postProcessSettings.x = i;

          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
      }
    }

    if (ImGui::CollapsingHeader("Environment Map"))
    {
      auto ambient = Renderer3D::getStorage()->currentEnvironment.get();

      int skyboxWidth = state->skyboxWidth;
      int irradianceWidth = state->irradianceWidth;
      int prefilterWidth = state->prefilterWidth;
      int prefilterSamples = state->prefilterSamples;

      if (ambient->getDrawingType() != MapType::DynamicSky)
      {
          if (ImGui::Button("Recompute Environment Map"))
          {
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
              if (skyboxWidth - (int)state->skyboxWidth < 0)
              {
                  skyboxWidth = std::pow(2, std::floor(std::log2(skyboxWidth)));
              }
              else if (skyboxWidth - (int)state->skyboxWidth > 0)
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
              if (irradianceWidth - (int)state->irradianceWidth < 0)
              {
                  irradianceWidth = std::pow(2, std::floor(std::log2(irradianceWidth)));
              }
              else if (irradianceWidth - (int)state->irradianceWidth > 0)
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
              if (prefilterWidth - (int)state->prefilterWidth < 0)
              {
                  prefilterWidth = std::pow(2, std::floor(std::log2(prefilterWidth)));
              }
              else if (prefilterWidth - (int)state->prefilterWidth > 0)
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
              if (prefilterSamples - (int)state->prefilterSamples < 0)
              {
                  prefilterSamples = std::pow(2, std::floor(std::log2(prefilterSamples)));
              }
              else if (prefilterSamples - (int)state->prefilterSamples > 0)
              {
                  prefilterSamples = std::pow(2, std::floor(std::log2(prefilterSamples)) + 1);
              }

              prefilterSamples = std::pow(2, std::floor(std::log2(prefilterSamples)));
              state->prefilterSamples = prefilterSamples;
          }
      }

      if (ambient->getDrawingType() == MapType::DynamicSky)
      {
          if (ambient->getDynamicSkyType() == DynamicSkyType::Preetham)
          {
              ImGui::Text("");
              ImGui::Separator();
              ImGui::Text("Preetham LUT");
              ImGui::Image((ImTextureID)(unsigned long)ambient->getTexID(MapType::DynamicSky),
                  ImVec2(256.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
          }
          if (ambient->getDynamicSkyType() == DynamicSkyType::Hillaire)
          {
              ImGui::Text("");
              ImGui::Separator();
              ImGui::Text("Transmittance and Multi-Scatter LUTs");
              ImGui::Image((ImTextureID)(unsigned long)ambient->getTransmittanceLUTID(),
                  ImVec2(256.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
              ImGui::SameLine();
              ImGui::Image((ImTextureID)(unsigned long)ambient->getMultiScatteringLUTID(),
                  ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
              ImGui::Text("Skyview LUT");
              ImGui::Image((ImTextureID)(unsigned long)ambient->getTexID(MapType::DynamicSky),
                  ImVec2(256.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
          }
      }
    }

    if (ImGui::CollapsingHeader("Render Passes"))
    {
      auto bufferSize = storage->gBuffer.getSize();
      float ratio = bufferSize.x / bufferSize.y;

      ImGui::Separator();
      ImGui::Text("Lighting:");
      ImGui::Separator();
      ImGui::Image((ImTextureID) (unsigned long) storage->lightingPass.getAttachID(FBOTargetParam::Colour0),
      ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Separator();
      ImGui::Text("GBuffer:");
      ImGui::Separator();
      ImGui::Text("Positions:");
      ImGui::Image((ImTextureID) (unsigned long) storage->gBuffer.getAttachmentID(FBOTargetParam::Colour0),
      ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Normals:");
      ImGui::Image((ImTextureID) (unsigned long) storage->gBuffer.getAttachmentID(FBOTargetParam::Colour1),
      ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Albedo:");
      ImGui::Image((ImTextureID) (unsigned long) storage->gBuffer.getAttachmentID(FBOTargetParam::Colour2),
      ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Materials:");
      ImGui::Image((ImTextureID) (unsigned long) storage->gBuffer.getAttachmentID(FBOTargetParam::Colour3),
      ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Entity Mask:");
      ImGui::Image((ImTextureID) (unsigned long) storage->gBuffer.getAttachmentID(FBOTargetParam::Colour4),
      ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Depth:");
      ImGui::Image((ImTextureID) (unsigned long) storage->gBuffer.getAttachmentID(FBOTargetParam::Depth),
      ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
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
