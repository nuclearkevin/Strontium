#include "Graphics/RenderPasses/BloomPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  BloomPass::BloomPass(Renderer3D::GlobalRendererData* globalRendererData)
	: RenderPass(&this->passData, globalRendererData, { })
	, timer(5)
  { }

  BloomPass::~BloomPass()
  { }
  
  void BloomPass::onInit()
  {
    this->passData.bloomDownsampleKaris = ShaderCache::getShader("bloom_downsample_karis");
    this->passData.bloomDownsample = ShaderCache::getShader("bloom_downsample");
    this->passData.bloomCopy = ShaderCache::getShader("bloom_copy");
    this->passData.bloomUpsampleBlend = ShaderCache::getShader("bloom_upsample_blend");

	Texture2DParams bloomParams = Texture2DParams();
    bloomParams.sWrap = TextureWrapParams::ClampEdges;
    bloomParams.tWrap = TextureWrapParams::ClampEdges;
    bloomParams.minFilter = TextureMinFilterParams::LinearMipMapLinear;
    bloomParams.internal = TextureInternalFormats::RGBA16f;
    bloomParams.format = TextureFormats::RGBA;
    bloomParams.dataType = TextureDataType::Floats;

    this->passData.downsampleBuffer.setSize(1600 / 2, 900 / 2);
    this->passData.downsampleBuffer.setParams(bloomParams);
    this->passData.downsampleBuffer.initNullTexture();
    this->passData.downsampleBuffer.generateMips();

    this->passData.upsampleBuffer1.setSize(1600 / 2, 900 / 2);
    this->passData.upsampleBuffer1.setParams(bloomParams);
    this->passData.upsampleBuffer1.initNullTexture();
    this->passData.upsampleBuffer1.generateMips();

    this->passData.upsampleBuffer2.setSize(1600 / 2, 900 / 2);
    this->passData.upsampleBuffer2.setParams(bloomParams);
    this->passData.upsampleBuffer2.initNullTexture();
    this->passData.upsampleBuffer2.generateMips();
  }

  void 
  BloomPass::updatePassData()
  { }

  RendererDataHandle 
  BloomPass::requestRendererData()
  {
    return -1;
  }

  void 
  BloomPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  BloomPass::onRendererBegin(uint width, uint height)
  {
    uint hWidth = static_cast<uint>(glm::ceil(static_cast<float>(width) / 2.0f));
	uint hHeight = static_cast<uint>(glm::ceil(static_cast<float>(height) / 2.0f));

    if (static_cast<uint>(this->passData.downsampleBuffer.getWidth()) != hWidth ||
        static_cast<uint>(this->passData.downsampleBuffer.getHeight()) != hHeight)
    {
      this->passData.downsampleBuffer.setSize(hWidth, hHeight);
      this->passData.downsampleBuffer.initNullTexture();
      this->passData.downsampleBuffer.generateMips();
      
      this->passData.upsampleBuffer1.setSize(hWidth, hHeight);
      this->passData.upsampleBuffer1.initNullTexture();
      this->passData.upsampleBuffer1.generateMips();
      
      this->passData.upsampleBuffer2.setSize(hWidth, hHeight);
      this->passData.upsampleBuffer2.initNullTexture();
      this->passData.upsampleBuffer2.generateMips();
    }
  }

  void 
  BloomPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    if (!this->passData.useBloom)
      return;

    // Set the bloom parameters;
    glm::vec4 curve(this->passData.threshold, this->passData.threshold - this->passData.knee, 
                    2.0f * this->passData.knee, 0.25 / this->passData.knee);
    this->passData.bloomParams.bindToPoint(0);
    this->passData.bloomParams.setData(0, sizeof(glm::vec4), &(curve.x));
    this->passData.bloomParams.setData(sizeof(glm::vec4), sizeof(float), &this->passData.radius);

    // Bind the lighting pass image.
    this->globalBlock->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::Read);

    // Bind the downsampling buffer.
    this->passData.downsampleBuffer.bindAsImage(1, 0, ImageAccessPolicy::Write);

    // Threshold and perform a Karis filter on the lighting pass to reduce fireflies.
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.downsampleBuffer.getWidth())
                                              / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.downsampleBuffer.getHeight())
                                               / 8.0f));
    this->passData.bloomDownsampleKaris->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    // Continuously downsample the bloom texture to make an image pyramid.
    float powerOfTwo = 2.0f;
    for (uint i = 1; i < MAX_NUM_BLOOM_MIPS; i++)
    {
      this->passData.downsampleBuffer.bindAsImage(0, i - 1, ImageAccessPolicy::Read);
      this->passData.downsampleBuffer.bindAsImage(1, i, ImageAccessPolicy::Write);
      iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.downsampleBuffer.getWidth())
                                           / (powerOfTwo * 8.0f)));
      iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.downsampleBuffer.getHeight())
                                            / (powerOfTwo * 8.0f)));
      this->passData.bloomDownsample->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
      powerOfTwo *= 2.0f;
    }

    // Copy the last mip of the downsampling chain to the last mip of the upsampling chain.
    powerOfTwo = std::pow(2.0, static_cast<float>(MAX_NUM_BLOOM_MIPS - 1));
    this->passData.downsampleBuffer.bindAsImage(0, MAX_NUM_BLOOM_MIPS - 1, ImageAccessPolicy::Read);
    this->passData.upsampleBuffer1.bindAsImage(1, MAX_NUM_BLOOM_MIPS - 1, ImageAccessPolicy::Write);
    iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.downsampleBuffer.getWidth())
                                         / (powerOfTwo * 8.0f)));
    iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.downsampleBuffer.getHeight())
                                          / (powerOfTwo * 8.0f)));
    this->passData.bloomCopy->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    // Blend the previous mips together with a blur on the previous mip.
    bool doubleBuffer = false;
    for (uint i = MAX_NUM_BLOOM_MIPS - 1; i > 0; i--)
    {
      powerOfTwo /= 2.0f;
      if (doubleBuffer)
      {
        this->passData.upsampleBuffer1.bind(0);
        this->passData.upsampleBuffer2.bindAsImage(1, i - 1, ImageAccessPolicy::Write);
      }
      else
      {
        this->passData.upsampleBuffer2.bind(0);
        this->passData.upsampleBuffer1.bindAsImage(1, i - 1, ImageAccessPolicy::Write);
      }
      this->passData.downsampleBuffer.bindAsImage(0, i - 1, ImageAccessPolicy::Read);

      float lod = static_cast<float>(i);
      this->passData.bloomParams.setData(sizeof(glm::vec4) + sizeof(float), sizeof(float), &lod);

      iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.downsampleBuffer.getWidth())
                                           / (powerOfTwo * 8.0f)));
      iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.downsampleBuffer.getHeight())
                                            / (powerOfTwo * 8.0f)));
      this->passData.bloomUpsampleBlend->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
      doubleBuffer = !doubleBuffer;
    }
  }

  void 
  BloomPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  BloomPass::onShutdown()
  {

  }
}