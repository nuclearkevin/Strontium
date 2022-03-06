#include "Graphics/RenderPasses/DirectionalLightPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  DirectionalLightPass::DirectionalLightPass(Renderer3D::GlobalRendererData* globalRendererData, 
                                             GeometryPass* previousGeoPass, 
                                             ShadowPass* previousShadowPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousShadowPass })
    , timer(5)
    , previousGeoPass(previousGeoPass)
    , previousShadowPass(previousShadowPass)
  { }

  DirectionalLightPass::~DirectionalLightPass()
  { }

  void 
  DirectionalLightPass::onInit()
  {
    this->passData.directionalEvaluation = ShaderCache::getShader("directional_evaluation");
    this->passData.directionalEvaluationS = ShaderCache::getShader("directional_evaluation_s");
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
    this->passData.castShadows = false;
  }

  void 
  DirectionalLightPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    // Bind the GBuffer attachments.
    auto& gBuffer = geometryBlock->gBuffer;
    gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
    gBuffer.bindAttachment(FBOTargetParam::Colour0, 1);
    gBuffer.bindAttachment(FBOTargetParam::Colour1, 2);
    gBuffer.bindAttachment(FBOTargetParam::Colour2, 3);

    // Bind the camera block.
    geometryBlock->cameraBuffer.bindToPoint(0);

    // Bind the lighting block and upload the directional lights.
    this->passData.lightBlock.bindToPoint(1);
    this->passData.lightBlock.setData(0, this->passData.directionalLightCount * sizeof(DirectionalLight), 
                                      this->passData.directionalLightQueue.data());
    int count = static_cast<int>(this->passData.directionalLightCount);
    this->passData.lightBlock.setData(MAX_NUM_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight), sizeof(int), &count);

    // Bind the lighting buffer.
    this->globalBlock->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::ReadWrite);

    // Launch a compute pass for unshadowed directional lights.
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(globalBlock->lightingBuffer.getWidth())
                                              / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(globalBlock->lightingBuffer.getHeight())
                                               / 8.0f));
    this->passData.directionalEvaluation->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

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

      glm::vec4 shadowParams(shadowBlock->constBias, shadowBlock->normalBias, shadowBlock->minRadius, 0.0f);
      this->passData.cascadedShadowBlock.setData(NUM_CASCADES * (sizeof(glm::mat4) + sizeof(glm::vec4)), 
                                                 sizeof(glm::vec4), &(shadowParams.x));

      // Bind the shadowbuffers.
      for (uint i = 0; i < NUM_CASCADES; i++)
        shadowBlock->shadowBuffers[i].bindTextureID(FBOTargetParam::Depth, 4 + i);

      this->passData.directionalEvaluationS->launchCompute(iWidth, iHeight, 1);
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
  DirectionalLightPass::submit(const DirectionalLight& light, const glm::mat4& model)
  {
    auto invTrans = glm::transpose(glm::inverse(model));
    DirectionalLight temp = light;
    temp.directionSize = glm::vec4(-1.0f * glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)), light.directionSize.w);

    this->passData.directionalLightQueue[this->passData.directionalLightCount] = temp;
    this->passData.directionalLightCount++;
  }

  void 
  DirectionalLightPass::submitPrimary(const DirectionalLight& light, bool castShadows, 
                                      const glm::mat4& model)
  {
    auto invTrans = glm::transpose(glm::inverse(model));
    DirectionalLight temp = light;
    temp.directionSize = glm::vec4(-1.0f * glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)), light.directionSize.w);

    this->passData.primaryLight = temp;
    this->passData.castShadows = castShadows;
  }
}