#include "Graphics/RenderPasses/HiZPass.h"

// Project includes.
#include "Graphics/RenderPasses/GeometryPass.h"

namespace Strontium
{
  HiZPass::HiZPass(Renderer3D::GlobalRendererData* globalRendererData, 
				   GeometryPass* previousGeoPass)
	: RenderPass(&this->passData, globalRendererData, { previousGeoPass })
	, previousGeoPass(previousGeoPass)
  { }

  HiZPass::~HiZPass()
  { }
  
  void 
  HiZPass::updatePassData()
  { }
  
  RendererDataHandle
  HiZPass::requestRendererData()
  {
	return -1;
  }

  void 
  HiZPass::deleteRendererData(const RendererDataHandle& handle)
  { }

  void 
  HiZPass::onInit()
  {
	this->passData.depthCopy = ShaderCache::getShader("copy_depth_hi_z");
	this->passData.depthReduction = ShaderCache::getShader("generate_hi_z");

	Texture2DParams depthParams = Texture2DParams();
	depthParams.sWrap = TextureWrapParams::ClampEdges;
	depthParams.tWrap = TextureWrapParams::ClampEdges;
	depthParams.internal = TextureInternalFormats::R32f;
	depthParams.format = TextureFormats::Red;
	depthParams.dataType = TextureDataType::Floats;
	depthParams.minFilter = TextureMinFilterParams::NearestMipMapNearest;
	depthParams.maxFilter = TextureMaxFilterParams::Nearest;
	this->passData.hierarchicalDepth.setSize(1600, 900);
	this->passData.hierarchicalDepth.setParams(depthParams);
	this->passData.hierarchicalDepth.initNullTexture();
	this->passData.hierarchicalDepth.generateMips();
  }

  void 
  HiZPass::onRendererBegin(uint width, uint height)
  {
	uint dWidth = static_cast<uint>(glm::ceil(this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()->gBuffer.getSize().x / 8.0f));
	uint dHeight = static_cast<uint>(glm::ceil(this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()->gBuffer.getSize().y / 8.0f));

    if (dWidth != width || dHeight != height)
	{
	  this->passData.hierarchicalDepth.setSize(width, height);
	  this->passData.hierarchicalDepth.initNullTexture();
	  this->passData.hierarchicalDepth.generateMips();
	}
  }

  void 
  HiZPass::onRender()
  {
	auto start = std::chrono::steady_clock::now();

	// Copy the depth from the depth buffer to the hi-z texture.
	this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()->gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
	this->passData.hierarchicalDepth.bindAsImage(0, 0, ImageAccessPolicy::Write);
	uint iWidth = static_cast<uint>(glm::ceil(this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()->gBuffer.getSize().x / 8.0f));
	uint iHeight = static_cast<uint>(glm::ceil(this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()->gBuffer.getSize().y / 8.0f));
	this->passData.depthCopy->launchCompute(iWidth, iHeight, 1);
	Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

	// Reduce the depth buffer.
	int maxDim = glm::max(this->passData.hierarchicalDepth.getWidth(),
						  this->passData.hierarchicalDepth.getHeight());
	uint numPasses = static_cast<uint>(glm::ceil(glm::log2(static_cast<float>(maxDim))));
	float powerOfTwo = 2.0f;
	for (uint i = 1; i < numPasses; i++)
	{
	  this->passData.hierarchicalDepth.bindAsImage(0, i, ImageAccessPolicy::Write);
	  this->passData.hierarchicalDepth.bindAsImage(1, i - 1, ImageAccessPolicy::Read);
	  iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.hierarchicalDepth.getWidth()) / (8.0f * powerOfTwo)));
	  iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.hierarchicalDepth.getHeight()) / (8.0f * powerOfTwo)));
	  this->passData.depthReduction->launchCompute(iWidth, iHeight, 1);
	  Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
	  powerOfTwo *= 2.0f;
	}

	auto end = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = end - start;
    this->passData.cpuTime = elapsed.count() * 1000.0f;
  }

  void 
  HiZPass::onRendererEnd(FrameBuffer& frontBuffer)
  { }

  void 
  HiZPass::onShutdown()
  { }
}