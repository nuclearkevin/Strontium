#include "Graphics/RenderPasses/SkyboxPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  SkyboxPass::SkyboxPass(Renderer3D::GlobalRendererData* globalRendererData, 
                         GeometryPass* previousGeoPass,
			             SkyAtmospherePass* previousSkyAtmoPass)
    : RenderPass(&passData, globalRendererData, { previousGeoPass, previousSkyAtmoPass })
    , timer(5)
    , previousGeoPass(previousGeoPass)
    , previousSkyAtmoPass(previousSkyAtmoPass)
  { }

  SkyboxPass::~SkyboxPass()
  { }

  void 
  SkyboxPass::onInit()
  {
    this->passData.dynamicSkyEvaluation = ShaderCache::getShader("dynamic_sky_evaluation");
  }

  void 
  SkyboxPass::updatePassData()
  { }

  RendererDataHandle 
  SkyboxPass::requestRendererData()
  {
    return -1;
  }

  void 
  SkyboxPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  SkyboxPass::onRendererBegin(uint width, uint height)
  {
    this->passData.skyboxes.clear();
  }

  void 
  SkyboxPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    if (!(this->passData.skyboxes.size() > 0u))
      return;

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);

    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    // Bind the depth GBuffer attachment.
    geometryBlock->gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
    // Bind the camera block.
    rendererData->cameraBuffer.bindToPoint(0);

    auto dynamicSkyBlock = this->previousSkyAtmoPass->getInternalDataBlock<SkyAtmospherePassDataBlock>();
    // Bind the dynamic sky buffers required to apply the skybox.
    dynamicSkyBlock->transmittanceLUTs.bind(1);
    dynamicSkyBlock->skyviewLUTs.bind(2);
    dynamicSkyBlock->atmosphereBuffer.bindToPoint(1);

    // Bind and upload the skybox properties.
    this->passData.skyboxParams.bindToPoint(2);
    // TODO: Handle multiple skyboxes.
    this->passData.skyboxParams.setData(0, sizeof(glm::vec4), &(this->passData.skyboxes[0])); 

    // Apply the skybox.
    rendererData->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::ReadWrite);
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getWidth())
                                              / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getHeight())
                                               / 8.0f));
    this->passData.dynamicSkyEvaluation->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
  }

  void 
  SkyboxPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  SkyboxPass::onShutdown()
  { }

  void 
  SkyboxPass::submit(RendererDataHandle skyAtmosphereIndex, float sunSize, float skyIntensity)
  {
    if (skyAtmosphereIndex >= 0)
      this->passData.skyboxes.emplace_back(static_cast<float>(skyAtmosphereIndex), 
                                           sunSize, skyIntensity, 0.0f);
  }
}