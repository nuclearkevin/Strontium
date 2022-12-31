#include "Graphics/RenderPasses/AreaLightPass.h"

// Project includes.
#include "Graphics/Renderer.h"

// LTC LUTs.
#include "Graphics/LTCLUTs.h"

namespace Strontium
{
  AreaLightPass::AreaLightPass(Renderer3D::GlobalRendererData* globalRendererData,
                               GeometryPass* previousGeoPass, LightCullingPass* previousCullPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass })
    , timer(5)
    , previousGeoPass(previousGeoPass)
    , previousCullPass(previousCullPass)
  { }
  
  AreaLightPass::~AreaLightPass()
  { }
  
  void 
  AreaLightPass::onInit()
  {
    this->passData.rectAreaLight = ShaderCache::getShader("rect_area_evaluation");

    // LTC LUTs here.
    Texture2DParams LUTParams = Texture2DParams();
    LUTParams.sWrap = TextureWrapParams::ClampEdges;
    LUTParams.tWrap = TextureWrapParams::ClampEdges;
    LUTParams.internal = TextureInternalFormats::RGBA32f;
    LUTParams.dataType = TextureDataType::Floats;
    LUTParams.minFilter = TextureMinFilterParams::Nearest;
    LUTParams.dataType = TextureDataType::Floats;

    this->passData.ltcLUT1.setSize(64u, 64u);
    this->passData.ltcLUT1.setParams(LUTParams);
    this->passData.ltcLUT1.loadData(LTC1);

    this->passData.ltcLUT2.setSize(64u, 64u);
    this->passData.ltcLUT2.setParams(LUTParams);
    this->passData.ltcLUT2.loadData(LTC2);
  }

  void 
  AreaLightPass::updatePassData()
  { }

  RendererDataHandle 
  AreaLightPass::requestRendererData()
  {
    return -1;
  }

  void 
  AreaLightPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  AreaLightPass::onRendererBegin(uint width, uint height)
  { }

  void 
  AreaLightPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    // Early out if there are no lights.
    auto cullingBlock = this->previousCullPass->getInternalDataBlock<LightCullingPassDataBlock>();

    // Early out if there are no area lights.
    if (cullingBlock->rectAreaLightCount == 0u)
      return;

    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    // Bind the GBuffer attachments.
    auto& gBuffer = geometryBlock->gBuffer;
    gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
    gBuffer.bindAttachment(FBOTargetParam::Colour0, 1);
    gBuffer.bindAttachment(FBOTargetParam::Colour1, 2);
    gBuffer.bindAttachment(FBOTargetParam::Colour2, 3);

    // Bind the camera block.
    static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock)->cameraBuffer.bindToPoint(0);

    // Bind the lighting buffer.
    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);
    rendererData->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::ReadWrite);
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getWidth())
                                              / 16.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getHeight())
                                               / 16.0f));
    
    // Bind the lighting block and LTC LUTs.
    this->passData.ltcLUT1.bind(4);
    this->passData.ltcLUT2.bind(5);
    {
      cullingBlock->rectLights.bindToPoint(1);
      cullingBlock->rectLightListBuffer.bindToPoint(0);

      // Launch a compute pass for the rectangular area lights.
      this->passData.rectAreaLight->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }
  }

  void 
  AreaLightPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  AreaLightPass::onShutdown()
  { }
}