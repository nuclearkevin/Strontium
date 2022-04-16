#include "Graphics/RenderPasses/DirectionalLightPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  DirectionalLightPass::DirectionalLightPass(Renderer3D::GlobalRendererData* globalRendererData, 
                                             GeometryPass* previousGeoPass, 
                                             ShadowPass* previousShadowPass,
                                             SkyAtmospherePass* previousSkyAtmoPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousShadowPass })
    , timer(5)
    , previousGeoPass(previousGeoPass)
    , previousShadowPass(previousShadowPass)
    , previousSkyAtmoPass(previousSkyAtmoPass)
  { }

  DirectionalLightPass::~DirectionalLightPass()
  { }

  void 
  DirectionalLightPass::onInit()
  {
    this->passData.directionalEvaluationS = ShaderCache::getShader("directional_evaluation_s");
    this->passData.directionalEvaluationSA = ShaderCache::getShader("directional_evaluation_s_a");
    this->passData.directionalEvaluation = ShaderCache::getShader("directional_evaluation");
    this->passData.directionalEvaluationA = ShaderCache::getShader("directional_evaluation_a");
  }

  void 
  DirectionalLightPass::updatePassData()
  { }

  RendererDataHandle 
  DirectionalLightPass::requestRendererData()
  {
    return -1;
  }

  void 
  DirectionalLightPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  DirectionalLightPass::onRendererBegin(uint width, uint height)
  {
    this->passData.directionalLightCount = 0u;
    this->passData.directionalLightCountA = 0u;
    this->passData.castShadows = false;
  }

  void 
  DirectionalLightPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    // Early out if there are no directional lights.
    if (this->passData.directionalLightCount + this->passData.directionalLightCountA == 0
        && !this->passData.castShadows)
      return;

    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    // Bind the GBuffer attachments.
    auto& gBuffer = geometryBlock->gBuffer;
    gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
    gBuffer.bindAttachment(FBOTargetParam::Colour0, 1);
    gBuffer.bindAttachment(FBOTargetParam::Colour1, 2);
    gBuffer.bindAttachment(FBOTargetParam::Colour2, 3);

    // Bind the camera block.
    geometryBlock->cameraBuffer.bindToPoint(0);

    // Bind the lighting buffer.
    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);
    rendererData->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::ReadWrite);
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getWidth())
                                              / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getHeight())
                                               / 8.0f));

    // Bind the lighting block and upload the directional lights.
    this->passData.lightBlock.bindToPoint(1);
    if (this->passData.directionalLightCount > 0)
    {
      this->passData.lightBlock.setData(0, this->passData.directionalLightCount * sizeof(DirectionalLight),
                                        this->passData.directionalLightQueue.data());
      int count = static_cast<int>(this->passData.directionalLightCount);
      this->passData.lightBlock.setData(MAX_NUM_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight), sizeof(int), &count);

      // Launch a compute pass for unshadowed directional lights with no atmospheric effects.
      this->passData.directionalEvaluation->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }

    // Bind atmosphere parameters if we have directional lights that need atmospheric effects.
    if (this->passData.directionalLightCountA > 0
        || this->passData.primaryLightAttachedAtmo > -1)
    {
      auto skyAtmBlock = this->previousSkyAtmoPass->getInternalDataBlock<SkyAtmospherePassDataBlock>();
      skyAtmBlock->atmosphereBuffer.bindToPoint(3);
      skyAtmBlock->transmittanceLUTs.bind(8);
    }

    // Launch a compute pass for unshadowed directional lights with atmospheric effects.
    if (this->passData.directionalLightCountA > 0)
    {
      // Upload the data for the directional lights with atmospheric effects.
      this->passData.lightBlock.setData(0, this->passData.directionalLightCountA * sizeof(DirectionalLight),
                                        this->passData.directionalLightQueueA.data());
      int count = static_cast<int>(this->passData.directionalLightCountA);
      this->passData.lightBlock.setData(MAX_NUM_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight), sizeof(int), &count);

      // Bind and upload the atmosphere indices.
      this->passData.atmosphereIndices.bindToPoint(4);
      this->passData.atmosphereIndices.setData(0, this->passData.directionalLightCountA * sizeof(int), 
                                               this->passData.attachedAtmo.data());

      this->passData.directionalEvaluationA->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }

    // Shadowed directional light pass.
    if (this->passData.castShadows)
    {
      // Upload and bind the lightspace properties and shadow params.
      auto shadowBlock = this->previousShadowPass->getInternalDataBlock<ShadowPassDataBlock>();
      int shadowQuality = static_cast<int>(shadowBlock->shadowQuality);
      this->passData.lightBlock.setData(0, sizeof(DirectionalLight), &this->passData.primaryLight);
      this->passData.lightBlock.setData(MAX_NUM_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight) 
                                        + 3 * sizeof(int), sizeof(int), &shadowQuality);

      this->passData.cascadedShadowBlock.bindToPoint(2);
      this->passData.cascadedShadowBlock.setData(0, NUM_CASCADES * sizeof(glm::mat4), 
                                                 shadowBlock->cascades);
      this->passData.cascadedShadowBlock.setData(NUM_CASCADES * sizeof(glm::mat4), 
                                                 NUM_CASCADES * sizeof(glm::vec4), 
                                                 shadowBlock->cascadeSplits);

      glm::vec4 shadowParams(shadowBlock->constBias, shadowBlock->normalBias, shadowBlock->minRadius, shadowBlock->blendFraction);
      this->passData.cascadedShadowBlock.setData(NUM_CASCADES * (sizeof(glm::mat4) + sizeof(glm::vec4)), 
                                                 sizeof(glm::vec4), &(shadowParams.x));

      // Bind the shadowbuffers.
      for (uint i = 0; i < NUM_CASCADES; i++)
        shadowBlock->shadowBuffers[i].bindTextureID(FBOTargetParam::Depth, 4 + i);

      // No atmospheric effects.
      if (!(this->passData.primaryLightAttachedAtmo > -1))
        this->passData.directionalEvaluationS->launchCompute(iWidth, iHeight, 1);
      else
      {
        // Bind and upload the atmosphere indices.
        this->passData.atmosphereIndices.bindToPoint(4);
        this->passData.atmosphereIndices.setData(0, sizeof(int), &this->passData.primaryLightAttachedAtmo);

        this->passData.directionalEvaluationSA->launchCompute(iWidth, iHeight, 1);
      }
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }
  }

  void 
  DirectionalLightPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  DirectionalLightPass::onShutdown()
  {

  }

  void 
  DirectionalLightPass::submit(const DirectionalLight& light, const glm::mat4& model,
                               RendererDataHandle attachedSkyAtmosphere)
  {
    auto invTrans = glm::transpose(glm::inverse(model));
    DirectionalLight temp = light;
    temp.directionSize = glm::vec4(-1.0f * glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)), light.directionSize.w);

    if (attachedSkyAtmosphere > -1)
    {
      this->passData.directionalLightQueueA[this->passData.directionalLightCountA] = temp;
      this->passData.attachedAtmo[this->passData.directionalLightCountA] = attachedSkyAtmosphere;
      this->passData.directionalLightCountA++;
    }
    else
    {
      this->passData.directionalLightQueue[this->passData.directionalLightCount] = temp;
      this->passData.directionalLightCount++;
    }
  }

  void 
  DirectionalLightPass::submitPrimary(const DirectionalLight& light, bool castShadows, 
                                      const glm::mat4& model, RendererDataHandle attachedSkyAtmosphere)
  {
    auto invTrans = glm::transpose(glm::inverse(model));
    DirectionalLight temp = light;
    temp.directionSize = glm::vec4(-1.0f * glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)), light.directionSize.w);

    this->passData.primaryLight = temp;
    this->passData.castShadows = castShadows;
    this->passData.primaryLightAttachedAtmo = attachedSkyAtmosphere;
  }
}