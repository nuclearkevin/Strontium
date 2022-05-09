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
    this->passData.ao.setSize(1600, 900);
    this->passData.ao.setParams(aoParams);
    this->passData.ao.initNullTexture();
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
    if (static_cast<uint>(this->passData.ao.getWidth()) != width ||
        static_cast<uint>(this->passData.ao.getHeight()) != height)
	{
	  this->passData.ao.setSize(width, height);
	  this->passData.ao.initNullTexture();
	}

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);
    this->passData.aoRadiusToScreen = 0.5f * this->passData.aoRadius * static_cast<float>(height) 
                                      / (2.0f * glm::tan(0.5f * rendererData->sceneCam.fov));
  }

  void HBAOPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);

    if (this->passData.enableAO)
    {
      struct HBAOBlockData
      {
        glm::vec4 aoParams1; // World-space radius (x), ao multiplier (y), ao exponent (z).
        glm::vec4 aoParams2; // Blur direction (x, y). Z and w are unused.
      }
        hbaoData
      {
        { this->passData.aoRadiusToScreen, this->passData.aoMultiplier,
          this->passData.aoExponent, 0.0f},
        { 1.0f, 0.0f, 0.0f, 0.0f }
      };
      
      // Populate the AO parameters.
      this->passData.aoParamsBuffer.setData(0, sizeof(HBAOBlockData), &hbaoData);

      // Bind the downsampled depth.
      auto hzBlock = this->previousHiZPass->getInternalDataBlock<HiZPassDataBlock>();
      hzBlock->hierarchicalDepth.bind(0);

      // Bind the camera buffer and the AO parameters.
      rendererData->cameraBuffer.bindToPoint(0);
      this->passData.aoParamsBuffer.bindToPoint(1);

      // Bind the blue noise texture.
      rendererData->spatialBlueNoise->bind(1);

      // Compute the SSHBAO.
      rendererData->halfResBuffer1.bindAsImage(0, 0, ImageAccessPolicy::Write);
      uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(hzBlock->hierarchicalDepth.getWidth())
                                                / 16.0f));
      uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(hzBlock->hierarchicalDepth.getHeight())
                                                 / 16.0f));
      this->passData.aoCompute->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

      // Blur the SSHBAO texture to get rid of noise from the jittering.
      rendererData->halfResBuffer1.bind(1);
      rendererData->fullResBuffer1.bindAsImage(0, 0, ImageAccessPolicy::Write);
      this->passData.aoBlur->launchCompute(2 * iWidth, 2 * iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

      hbaoData.aoParams2 = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
      this->passData.aoParamsBuffer.setData(sizeof(glm::vec4), sizeof(glm::vec4), &(hbaoData.aoParams2.x));
      rendererData->fullResBuffer1.bind(1);
      this->passData.ao.bindAsImage(0, 0, ImageAccessPolicy::Write);
      this->passData.aoBlur->launchCompute(2 * iWidth, 2 * iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }
  }

  void HBAOPass::onRendererEnd(FrameBuffer &frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void HBAOPass::onShutdown()
  {

  }
}