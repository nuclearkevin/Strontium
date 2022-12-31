#include "Graphics/RenderPasses/CulledLightingPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  CulledLightingPass::CulledLightingPass(Renderer3D::GlobalRendererData* globalRendererData,
                                         GeometryPass* previousGeoPass, 
                                         LightCullingPass* previousCullPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousCullPass })
    , previousGeoPass(previousGeoPass)
    , previousCullPass(previousCullPass)
    , timer(5u)
  { }

  CulledLightingPass::~CulledLightingPass()
  { }

  void 
  CulledLightingPass::onInit()
  {
    this->passData.evaluatePointLight = ShaderCache::getShader("point_evaluation");
  }

  void 
  CulledLightingPass::updatePassData()
  { }

  RendererDataHandle 
  CulledLightingPass::requestRendererData()
  {
    return -1;
  }

  void 
  CulledLightingPass::deleteRendererData(RendererDataHandle &handle)
  { }

  void 
  CulledLightingPass::onRendererBegin(uint width, uint height)
  { }

  void 
  CulledLightingPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    // Early out if there are no lights.
    auto cullingBlock = this->previousCullPass->getInternalDataBlock<LightCullingPassDataBlock>();

    uint numLights = 0u;
    numLights += cullingBlock->pointLightCount;

    if (numLights == 0u)
      return;

    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    // Bind the GBuffer attachments.
    auto& gBuffer = geometryBlock->gBuffer;
    gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
    gBuffer.bindAttachment(FBOTargetParam::Colour0, 1);
    gBuffer.bindAttachment(FBOTargetParam::Colour1, 2);
    gBuffer.bindAttachment(FBOTargetParam::Colour2, 3);

    // Bind the camera block.
    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);
    rendererData->cameraBuffer.bindToPoint(0);

    // Bind the lighting buffer.
    rendererData->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::ReadWrite);

    if (cullingBlock->pointLightCount > 0u)
    {
      // Point lights and culling data.
      cullingBlock->pointLights.bindToPoint(1u);
      cullingBlock->pointLightListBuffer.bindToPoint(2);

      // Launching in the lighting tile size as opposed to 8x8 groups.
      uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getWidth())
                                                / 16.0f));
      uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getHeight())
                                                 / 16.0f));
      this->passData.evaluatePointLight->launchCompute(iWidth, iHeight, 1u);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }
  }

  void 
  CulledLightingPass::onRendererEnd(FrameBuffer &frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  CulledLightingPass::onShutdown()
  { }
}