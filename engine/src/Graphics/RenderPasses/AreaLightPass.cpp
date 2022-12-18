#include "Graphics/RenderPasses/AreaLightPass.h"

// Project includes.
#include "Graphics/Renderer.h"

// LTC LUTs.
#include "Graphics/LTCLUTs.h"

namespace Strontium
{
  AreaLightPass::AreaLightPass(Renderer3D::GlobalRendererData* globalRendererData,
                               GeometryPass* previousGeoPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass })
    , timer(5)
    , previousGeoPass(previousGeoPass)
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
  {
    this->passData.rectAreaLightCount = 0u;
  }

  void 
  AreaLightPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    // Early out if there are no area lights.
    if (this->passData.rectAreaLightCount == 0u)
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
                                              / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getHeight())
                                               / 8.0f));
    
    // Bind the lighting block and LTC LUTs.
    this->passData.ltcLUT1.bind(4);
    this->passData.ltcLUT2.bind(5);
    this->passData.lightBlock.bindToPoint(1);
    // Upload the rectangular area lights.
    {
      this->passData.lightBlock.setData(0u, this->passData.rectAreaLightCount * sizeof(RectAreaLight), 
                                        this->passData.rectLightQueue.data());
      int count = static_cast<int>(this->passData.rectAreaLightCount);
      this->passData.lightBlock.setData(MAX_NUM_RECT_LIGHTS * sizeof(RectAreaLight), sizeof(int), &count);

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

  void 
  AreaLightPass::submit(const RectAreaLight &light, const glm::mat4 &model, bool twoSided)
  {
    if (this->passData.rectAreaLightCount > 7)
      return;

    this->passData.rectLightQueue[this->passData.rectAreaLightCount] = light;
    for (uint i = 0u; i < 4; ++i)
    {
      this->passData.rectLightQueue[this->passData.rectAreaLightCount].points[i] = 
        model * this->passData.rectLightQueue[this->passData.rectAreaLightCount].points[i];
      this->passData.rectLightQueue[this->passData.rectAreaLightCount].points[i].w = static_cast<float>(twoSided);
    }

    this->passData.rectAreaLightCount++;
  }
}