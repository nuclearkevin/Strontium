#include "Graphics/RenderPasses/HBAOPass.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/HiZPass.h"

namespace Strontium
{
  HBAOPass::HBAOPass(Renderer3D::GlobalRendererData* globalRendererData,
  	                 GeometryPass* previousGeoPass, HiZPass* previousHiZPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousHiZPass })
    , previousGeoPass(previousGeoPass)
    , previousHiZPass(previousHiZPass)
    , timer(5)
  { }

  HBAOPass::~HBAOPass()
  { }
  
  void HBAOPass::onInit()
  {
    this->passData.aoCompute = ShaderCache::getShader("screen_space_hbao");
    this->passData.aoBlur = ShaderCache::getShader("screen_space_hbao_blur");

    Texture2DParams aoParams = Texture2DParams();
    aoParams.sWrap = TextureWrapParams::ClampEdges;
    aoParams.tWrap = TextureWrapParams::ClampEdges;
    aoParams.internal = TextureInternalFormats::RGBA16f;
    aoParams.format = TextureFormats::RGBA;
    aoParams.dataType = TextureDataType::Floats;
    this->passData.downsampleAO.setSize(1600 / 2, 900 / 2);
    this->passData.downsampleAO.setParams(aoParams);
    this->passData.downsampleAO.initNullTexture();
  }

  void HBAOPass::updatePassData()
  { }

  RendererDataHandle 
  HBAOPass::requestRendererData()
  {
    return -1;
  }

  void 
  HBAOPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void HBAOPass::onRendererBegin(uint width, uint height)
  {
    uint hWidth = static_cast<uint>(glm::ceil(this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()->gBuffer.getSize().x / 2.0f));
	uint hHeight = static_cast<uint>(glm::ceil(this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()->gBuffer.getSize().y / 2.0f));

    if (static_cast<uint>(this->passData.downsampleAO.getWidth()) != hWidth || 
        static_cast<uint>(this->passData.downsampleAO.getHeight()) != hHeight)
	{
	  this->passData.downsampleAO.setSize(hWidth, hHeight);
	  this->passData.downsampleAO.initNullTexture();
	}
  }

  void HBAOPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    if (this->passData.enableAO)
    {
      struct HBAOBlockData
      {
        glm::vec4 aoParams1; // World-space radius (x), ao multiplier (y), ao exponent (z).
        glm::vec4 aoParams2; // Blur direction (x, y). Z and w are unused.
      }
        hbaoData
      {
        { this->passData.aoRadius, this->passData.aoMultiplier,
          this->passData.aoExponent, 0.0f},
        { 1.0f, 0.0f, 0.0f, 0.0f }
      };
      
      // Populate the AO parameters.
      this->passData.aoParamsBuffer.setData(0, sizeof(HBAOBlockData), &hbaoData);

      // Bind the downsampled depth.
      auto hzBlock = this->previousHiZPass->getInternalDataBlock<HiZPassDataBlock>();
      hzBlock->hierarchicalDepth.bind(0);

      // Bind the camera buffer and the AO parameters.
      this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()
                           ->cameraBuffer.bindToPoint(0);
      this->passData.aoParamsBuffer.bindToPoint(1);

      // Compute the SSHBAO.
      this->passData.downsampleAO.bindAsImage(0, 0, ImageAccessPolicy::Write);
      uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(hzBlock->hierarchicalDepth.getWidth())
                                                / 8.0f));
      uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(hzBlock->hierarchicalDepth.getHeight())
                                                 / 8.0f));
      this->passData.aoCompute->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

      // Blur the SSHBAO texture to get rid of noise from the jittering.
      this->passData.downsampleAO.bind(1);
      this->globalBlock->halfResBuffer1.bindAsImage(0, 0, ImageAccessPolicy::Write);
      this->passData.aoBlur->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

      hbaoData.aoParams2 = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
      this->passData.aoParamsBuffer.setData(sizeof(glm::vec4), sizeof(glm::vec4), &(hbaoData.aoParams2.x));
      this->globalBlock->halfResBuffer1.bind(1);
      this->passData.downsampleAO.bindAsImage(0, 0, ImageAccessPolicy::Write);
      this->passData.aoBlur->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }
  }

  void HBAOPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void HBAOPass::onShutdown()
  {

  }
}