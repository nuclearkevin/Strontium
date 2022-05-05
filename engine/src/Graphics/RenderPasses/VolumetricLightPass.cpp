#include "Graphics/RenderPasses/VolumetricLightPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  VolumetricLightPass::VolumetricLightPass(Renderer3D::GlobalRendererData* globalRendererData,
                                           GeometryPass* previousGeoPass, HiZPass* previousHiZPass, 
                                           GodrayPass* previousGodrayPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousHiZPass, previousGodrayPass })
    , previousGeoPass(previousGeoPass)
    , previousHiZPass(previousHiZPass)
    , previousGodrayPass(previousGodrayPass)
    , timer(5)
  { }

  VolumetricLightPass::~VolumetricLightPass()
  { }

  void 
  VolumetricLightPass::onInit()
  {
    this->passData.volumetricApplication = ShaderCache::getShader("volumetric_evaluation");
  }

  void 
  VolumetricLightPass::updatePassData()
  { }

  RendererDataHandle 
  VolumetricLightPass::requestRendererData()
  {
    return -1;
  }

  void 
  VolumetricLightPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  VolumetricLightPass::onRendererBegin(uint width, uint height)
  { }

  void 
  VolumetricLightPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    auto godrayBlock = this->previousGodrayPass->getInternalDataBlock<GodrayPassDataBlock>();

    if (!godrayBlock->hasGodrays)
      return;

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);
    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    auto hzBlock = this->previousHiZPass->getInternalDataBlock<HiZPassDataBlock>();

    // Bind the camera.
    geometryBlock->cameraBuffer.bindToPoint(0);

    // Set the volumetric flags.
    int passFlags = 0;
    if (godrayBlock->hasGodrays)
      passFlags |= (1 << 0);

    this->passData.volumetricParamBuffer.setData(0, sizeof(int), &passFlags);
    this->passData.volumetricParamBuffer.bindToPoint(1);

    // Bind the lighting buffer.
    rendererData->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::ReadWrite);

    // Bind the noise texture.
    rendererData->spatialBlueNoise->bind(2);

    // Bind the HiZ buffer and the volumetric texture.
    hzBlock->hierarchicalDepth.bind(0);
    godrayBlock->finalGather.bind(1);

    // Launch a compute pass to apply volumetric lighting.
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getWidth())
                                              / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getHeight())
                                               / 8.0f));
    this->passData.volumetricApplication->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
  }

  void 
  VolumetricLightPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  VolumetricLightPass::onShutdown()
  {

  }
}