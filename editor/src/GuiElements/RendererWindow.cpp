#include "GuiElements/RendererWindow.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/ShadowPass.h"
#include "Graphics/RenderPasses/HiZPass.h"
#include "Graphics/RenderPasses/HBAOPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"

#include "Graphics/RenderPasses/IBLApplicationPass.h"

#include "Graphics/RenderPasses/SkyboxPass.h"

#include "Graphics/RenderPasses/PostProcessingPass.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "GuiElements/Styles.h"

namespace Strontium
{
  RendererWindow::RendererWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , transmittanceLUTView("Transmittance LUT")
    , multiscatteringLUTView("Multiscattering LUT")
    , skyviewLUTView("Skyview LUT")
    , irradianceView("Dynamic Sky Irradiance")
    , radianceView("Dynamic Sky Radiance")
  { }

  RendererWindow::~RendererWindow()
  { }

  void
  RendererWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    ImGui::Begin("Renderer Settings", &isOpen);
    if (ImGui::CollapsingHeader("Shadow Pass"))
    {
      auto& renderPassManger = Renderer3D::getPassManager();
      auto shadowPass = renderPassManger.getRenderPass<ShadowPass>();
      auto shadowBlock = shadowPass->getInternalDataBlock<ShadowPassDataBlock>();

      ImGui::Text("Frametime: %f ms", shadowBlock->frameTime);
      ImGui::Text("Number of Draw Calls: %u", shadowBlock->numDrawCalls);
      ImGui::Text("Number of Instances: %u", shadowBlock->numInstances);
      ImGui::Text("Number of Triangles Submitted: %u", shadowBlock->numTrianglesSubmitted);
      ImGui::Text("Number of Triangles Drawn: %u", shadowBlock->numTrianglesDrawn);
      ImGui::Text("Number of Triangles Culled: %u", shadowBlock->numTrianglesSubmitted - shadowBlock->numTrianglesDrawn);

      ImGui::Separator();

      const char* shadowQualities[] = { "Hard Shadows", "Medium Quality (PCF)", "Ultra Quality (PCSS)" };

      if (ImGui::BeginCombo("##dirSettings", shadowQualities[shadowBlock->shadowQuality]))
      {
        for (uint i = 0; i < IM_ARRAYSIZE(shadowQualities); i++)
        {
          bool isSelected = (shadowQualities[i] == shadowQualities[shadowBlock->shadowQuality]);
          if (ImGui::Selectable(shadowQualities[i], isSelected))
            shadowBlock->shadowQuality = i;

          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
      }

      ImGui::DragFloat("Cascade Lambda", &shadowBlock->cascadeLambda, 0.01f, 0.5f, 1.0f);

      int shadowWidth = static_cast<int>(shadowBlock->shadowMapRes);
      if (ImGui::InputInt("Shadowmap Size", &shadowWidth))
      {
        shadowWidth = shadowWidth > 4096 ? 4096 : shadowWidth;
        shadowWidth = shadowWidth < 512 ? 512 : shadowWidth;

        // Compute the next or previous power of 2 and set the int to that.
        if (shadowWidth - static_cast<int>(shadowBlock->shadowMapRes) < 0)
        {
          shadowWidth = std::pow(2, std::floor(std::log2(shadowWidth)));
        }
        else if (shadowWidth - static_cast<int>(shadowBlock->shadowMapRes) > 0)
        {
          shadowWidth = std::pow(2, std::floor(std::log2(shadowWidth)) + 1);
        }

        shadowWidth = std::pow(2, std::floor(std::log2(shadowWidth)));
        shadowBlock->shadowMapRes = static_cast<uint>(shadowWidth);

        shadowPass->updatePassData();
      }

      /*
      ImGui::Text("");

      if (state->directionalSettings.x == 0)
      {
        float normalBias = state->shadowParams[0].w;
        if (ImGui::DragFloat("Normal Bias", &(normalBias), 0.01f))
        {
          state->shadowParams[0].w = glm::max(normalBias, 0.0f);
        }
        float constBias = state->shadowParams[1].x;
        if (ImGui::DragFloat("Constant Bias", &(constBias), 0.01f))
        {
          state->shadowParams[1].x = glm::max(constBias, 0.0f);
        }
      }

      if (state->directionalSettings.x == 1)
      {
        if (ImGui::DragFloat("Filter Radius", &(state->shadowParams[0].z), 0.01f))
        {
          state->shadowParams[0].z = glm::max(state->shadowParams[0].z, 0.0f);
        }
        float normalBias = state->shadowParams[0].w;
        if (ImGui::DragFloat("Normal Bias", &(normalBias), 0.01f))
        {
          state->shadowParams[0].w = glm::max(normalBias, 0.0f);
        }
        float constBias = state->shadowParams[1].x;
        if (ImGui::DragFloat("Constant Bias", &(constBias), 0.01f))
        {
          state->shadowParams[1].x = glm::max(constBias, 0.0f);
        }
      }

      if (state->directionalSettings.x == 2)
      {
        if (ImGui::DragFloat("Light Size", &(state->shadowParams[0].y), 0.01f))
        {
          state->shadowParams[0].y = glm::max(state->shadowParams[0].y, 0.0f);
        }
        if (ImGui::DragFloat("Minimum Radius", &(state->shadowParams[0].z), 0.01f))
        {
          state->shadowParams[0].z = glm::max(state->shadowParams[0].z, 0.0f);
        }
        float normalBias = state->shadowParams[0].w;
        if (ImGui::DragFloat("Normal Bias", &(normalBias), 0.01f))
        {
          state->shadowParams[0].w = glm::max(normalBias, 0.0f);
        }
        float constBias = state->shadowParams[1].x;
        if (ImGui::DragFloat("Constant Bias", &(constBias), 0.01f))
        {
          state->shadowParams[1].x = glm::max(constBias, 0.0f);
        }
      }
      */

      static bool showMaps = false;
      ImGui::Checkbox("Show shadow maps", &showMaps);

      if (showMaps)
      {
        static int cascadeIndex = 0;
        ImGui::SliderInt("Cascade Index", &cascadeIndex, 0, NUM_CASCADES - 1);
        ImGui::Text("Depth");
        ImGui::Image(reinterpret_cast<ImTextureID>(shadowBlock->shadowBuffers[static_cast<uint>(cascadeIndex)].getAttachID(FBOTargetParam::Depth)),
                     ImVec2(128.0f, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      }
    }

    /*
    if (ImGui::CollapsingHeader("Bloom"))
    {
      auto bufferSize = storage->gBuffer.getSize();
      float ratio = bufferSize.x / bufferSize.y;

      ImGui::Checkbox("Use Bloom", &state->enableBloom);
      ImGui::DragFloat("Threshold", &state->bloomThreshold, 0.01f, 0.0f, 10.0f);
      ImGui::DragFloat("Knee", &state->bloomKnee, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Radius", &state->bloomRadius, 0.01f, 0.0f, 10.0f);
      ImGui::DragFloat("Intensity", &state->bloomIntensity, 0.01f, 0.0f, 10.0f);

      static bool showBloomTextures = false;
      ImGui::Checkbox("Show Bloom Image Pyramid", &showBloomTextures);

      if (showBloomTextures)
      {
        ImGui::Text("Downsample Image Pyramid (%d, %d)",
                    storage->downscaleBloomTex.getWidth(),
                    storage->downscaleBloomTex.getHeight());
        ImGui::Image((ImTextureID) (unsigned long) storage->downscaleBloomTex.getID(),
                     ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::Text("Upsample Image Pyramid (%d, %d)",
                    storage->upscaleBloomTex.getWidth(),
                    storage->upscaleBloomTex.getHeight());
        ImGui::Image((ImTextureID) (unsigned long) storage->upscaleBloomTex.getID(),
                     ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::Text("Buffer Image Pyramid (%d, %d)",
                    storage->bufferBloomTex.getWidth(),
                    storage->bufferBloomTex.getHeight());
        ImGui::Image((ImTextureID) (unsigned long) storage->bufferBloomTex.getID(),
                     ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      }
    }
    */

    if (ImGui::CollapsingHeader("Geometry Pass"))
    {
      auto& renderPassManger = Renderer3D::getPassManager();
      auto geometryPass = renderPassManger.getRenderPass<GeometryPass>();
      auto geometryBlock = geometryPass->getInternalDataBlock<GeometryPassDataBlock>();
      auto& gBuffer = geometryBlock->gBuffer;

      auto bufferSize = gBuffer.getSize();
      float ratio = bufferSize.x / bufferSize.y;

      ImGui::Text("Frametime: %f ms", geometryBlock->frameTime);
      ImGui::Text("Number of Draw Calls: %u", geometryBlock->numDrawCalls);
      ImGui::Text("Number of Instances: %u", geometryBlock->numInstances);
      ImGui::Text("Number of Triangles Submitted: %u", geometryBlock->numTrianglesSubmitted);
      ImGui::Text("Number of Triangles Drawn: %u", geometryBlock->numTrianglesDrawn);
      ImGui::Text("Number of Triangles Culled: %u", geometryBlock->numTrianglesSubmitted - geometryBlock->numTrianglesDrawn);

      ImGui::Separator();
      ImGui::Text("GBuffer:");
      ImGui::Separator();
      ImGui::Text("Normals:");
      ImGui::Image(reinterpret_cast<ImTextureID>(gBuffer.getAttachmentID(FBOTargetParam::Colour0)),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Albedo:");
      ImGui::Image(reinterpret_cast<ImTextureID>(gBuffer.getAttachmentID(FBOTargetParam::Colour1)),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Materials:");
      ImGui::Image(reinterpret_cast<ImTextureID>(gBuffer.getAttachmentID(FBOTargetParam::Colour2)),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Entity Mask:");
      ImGui::Image(reinterpret_cast<ImTextureID>(gBuffer.getAttachmentID(FBOTargetParam::Colour3)),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::Text("Depth:");
      ImGui::Image(reinterpret_cast<ImTextureID>(gBuffer.getAttachmentID(FBOTargetParam::Depth)),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
    }

    if (ImGui::CollapsingHeader("Hi-Z Reduction Pass"))
    {
      auto& renderPassManger = Renderer3D::getPassManager();
      auto hiZPass = renderPassManger.getRenderPass<HiZPass>();
      auto hiZBlock = hiZPass->getInternalDataBlock<HiZPassDataBlock>();
      float width = static_cast<float>(hiZBlock->hierarchicalDepth.getWidth());
      float height = static_cast<float>(hiZBlock->hierarchicalDepth.getHeight());
      float ratio = width / height;

      ImGui::Text("Frametime: %f ms", hiZBlock->frameTime);

      ImGui::Separator();
      ImGui::Text("Hi-Z Buffer:");
      ImGui::Image(reinterpret_cast<ImTextureID>(hiZBlock->hierarchicalDepth.getID()),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
    }

    if (ImGui::CollapsingHeader("HBAO Pass"))
    {
      auto& renderPassManger = Renderer3D::getPassManager();
      auto hbaoPass = renderPassManger.getRenderPass<HBAOPass>();
      auto hbaoBlock = hbaoPass->getInternalDataBlock<HBAOPassDataBlock>();
      float width = static_cast<float>(hbaoBlock->downsampleAO.getWidth());
      float height = static_cast<float>(hbaoBlock->downsampleAO.getHeight());
      float ratio = width / height;

      ImGui::Checkbox("Enable HBAO", &hbaoBlock->enableAO);

      ImGui::Separator();
      ImGui::Text("Frametime: %f ms", hbaoBlock->frameTime);
      ImGui::Separator();

      ImGui::SliderFloat("Radius", &hbaoBlock->aoRadius, 0.0f, 0.1f);
      ImGui::SliderFloat("Multiplier", &hbaoBlock->aoMultiplier, 0.0f, 10.0f);
      ImGui::SliderFloat("Power", &hbaoBlock->aoExponent, 0.0f, 10.0f);

      static bool showHBAOTexture = false;
      ImGui::Checkbox("Show HBAO Texture", &showHBAOTexture);

      if (showHBAOTexture)
      {
        ImGui::Text("HBAO Texture");
        ImGui::Image(reinterpret_cast<ImTextureID>(hbaoBlock->downsampleAO.getID()),
                     ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
      }
    }

    if (ImGui::CollapsingHeader("Sky Atmosphere Pass"))
    {
      auto& renderPassManger = Renderer3D::getPassManager();
      auto skyAtmoPass = renderPassManger.getRenderPass<SkyAtmospherePass>();
      auto skyAtmoBlock = skyAtmoPass->getInternalDataBlock<SkyAtmospherePassDataBlock>();

      ImGui::Text("Frametime: %f ms", skyAtmoBlock->frameTime);

      ImGui::Checkbox("Use Fast Atmosphere", &(skyAtmoBlock->useFastAtmosphere));

      static bool showLUTs = false;
      ImGui::Checkbox("Show Hillaire LUTs", &showLUTs);
      if (showLUTs)
      {
        this->transmittanceLUTView.arrayImage(skyAtmoBlock->transmittanceLUTs, ImVec2(256.0f, 64.0f));
        this->multiscatteringLUTView.arrayImage(skyAtmoBlock->multiscatterLUTS, ImVec2(64.0f, 64.0f));
        this->skyviewLUTView.arrayImage(skyAtmoBlock->skyviewLUTs, ImVec2(256.0f, 128.0f));
      }
    }

    if (ImGui::CollapsingHeader("Dynamic Sky IBL Pass"))
    {
      auto& renderPassManger = Renderer3D::getPassManager();
      auto dynSkyIBLPass = renderPassManger.getRenderPass<DynamicSkyIBLPass>();
      auto dynIBLBlock = dynSkyIBLPass->getInternalDataBlock<DynamicSkyIBLPassDataBlock>();

      ImGui::Text("Frametime: %f ms", dynIBLBlock->frameTime);

      int irradSamples = static_cast<int>(dynIBLBlock->numIrradSamples);
      ImGui::SliderInt("Irradiance Samples", &irradSamples, 32, 1024, "%d", ImGuiSliderFlags_AlwaysClamp);
      dynIBLBlock->numIrradSamples = static_cast<uint>(irradSamples);

      int radSamples = static_cast<int>(dynIBLBlock->numRadSamples);
      ImGui::SliderInt("Radiance Samples", &radSamples, 32, 1024, "%d", ImGuiSliderFlags_AlwaysClamp);
      dynIBLBlock->numRadSamples = static_cast<uint>(radSamples);

      static bool showIBLMaps = false;
      ImGui::Checkbox("Show Irradiance and Radiance Maps", &showIBLMaps);
      if (showIBLMaps)
      {
        this->irradianceView.cubemapArrayImage(dynIBLBlock->irradianceCubemaps, ImVec2(192.0f, 128.0f));
        this->radianceView.cubemapArrayImage(dynIBLBlock->radianceCubemaps, ImVec2(192.0f, 128.0f), true);
      }
    }

    if (ImGui::CollapsingHeader("Lighting Pass"))
    {
      auto& globalBlock = Renderer3D::getStorage();
      auto& renderPassManger = Renderer3D::getPassManager();
      
      {
        auto iblAppPass = renderPassManger.getRenderPass<IBLApplicationPass>();
        auto iblAppBlock = iblAppPass->getInternalDataBlock<IBLApplicationPassDataBlock>();
        ImGui::Text("IBL Frametime: %f ms", iblAppBlock->frameTime);
      }
      {
        auto skyboxAppPass = renderPassManger.getRenderPass<SkyboxPass>();
        auto skyboxAppBlock = skyboxAppPass->getInternalDataBlock<SkyboxPassDataBlock>();
        ImGui::Text("Skybox Frametime: %f ms", skyboxAppBlock->frameTime);
      }

      ImGui::Text("Lighting Buffer");
      float ratio = static_cast<float>(globalBlock.lightingBuffer.getWidth()) 
                  / static_cast<float>(globalBlock.lightingBuffer.getHeight());
      ImGui::Image(reinterpret_cast<ImTextureID>(globalBlock.lightingBuffer.getID()),
                   ImVec2(128.0f * ratio, 128.0f), ImVec2(0, 1), ImVec2(1, 0));
    }

    if (ImGui::CollapsingHeader("General Post-Processing Pass"))
    {
      auto& globalBlock = Renderer3D::getStorage();
      auto& renderPassManger = Renderer3D::getPassManager();
      auto postPass = renderPassManger.getRenderPass<PostProcessingPass>();
      auto postBlock = postPass->getInternalDataBlock<PostProcessingPassDataBlock>();

      ImGui::Text("General Post-Processing Frametime: %f ms", postBlock->frameTime);

      ImGui::DragFloat("Gamma", &globalBlock.gamma, 0.01f, 1.0f, 10.0f);

      const char* toneMapOps[] = { "Reinhard", "Enhanced Reinhard", "Reinhard-Jodie", "Uncharted 2", "Fast ACES", "ACES", "None" };

      if (ImGui::BeginCombo("##toneMapCombo", toneMapOps[postBlock->toneMapOp]))
      {
        for (unsigned int i = 0; i < IM_ARRAYSIZE(toneMapOps); i++)
        {
          bool isSelected = (toneMapOps[i] == toneMapOps[postBlock->toneMapOp]);
          if (ImGui::Selectable(toneMapOps[i], isSelected))
              postBlock->toneMapOp = i;

          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
      }

      ImGui::Checkbox("Use FXAA", &postBlock->useFXAA);
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
