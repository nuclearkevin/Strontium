#include "Graphics/RenderPasses/IBLApplicationPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  IBLApplicationPass::IBLApplicationPass(Renderer3D::GlobalRendererData* globalRendererData,
										 GeometryPass* previousGeoPass,
                                         HiZPass* previousHiZPass,
                                         HBAOPass* previousHBAOPass,
                                         SkyAtmospherePass* previousSkyAtmoPass,
										 DynamicSkyIBLPass* previousDynSkyIBLPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousDynSkyIBLPass })
	, previousGeoPass(previousGeoPass)
    , previousHiZPass(previousHiZPass)
    , previousHBAOPass(previousHBAOPass)
    , previousSkyAtmoPass(previousSkyAtmoPass)
	, previousDynSkyIBLPass(previousDynSkyIBLPass)
	, timer(5)
  { }

  IBLApplicationPass::~IBLApplicationPass()
  { }

  void 
  IBLApplicationPass::onInit()
  {
    this->passData.iblEvaluation = ShaderCache::getShader("ibl_evaluation");
    this->passData.emissionEvaluation = ShaderCache::getShader("emission_evaluation");
    this->passData.brdfIntegration = ShaderCache::getShader("split_sums_brdf");

	Texture2DParams brdfLUTParams = Texture2DParams();
    brdfLUTParams.sWrap = TextureWrapParams::ClampEdges;
    brdfLUTParams.tWrap = TextureWrapParams::ClampEdges;
    brdfLUTParams.internal = TextureInternalFormats::RG16f;
    brdfLUTParams.dataType = TextureDataType::Floats;

    this->passData.brdfLUT.setSize(32, 32);
    this->passData.brdfLUT.setParams(brdfLUTParams);
    this->passData.brdfLUT.initNullTexture();

    // Bind the integration map for writing to by the compute shader.
    this->passData.brdfLUT.bindAsImage(0, 0, ImageAccessPolicy::Write);
    this->passData.brdfIntegration->launchCompute(4, 4, 1);
  }

  void 
  IBLApplicationPass::updatePassData()
  { }

  RendererDataHandle 
  IBLApplicationPass::requestRendererData()
  {
	return -1;
  }

  void 
  IBLApplicationPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  IBLApplicationPass::onRendererBegin(uint width, uint height)
  { }

  void 
  IBLApplicationPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);

    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    // Bind the GBuffer attachments.
    auto& gBuffer = geometryBlock->gBuffer;
    gBuffer.bindAttachment(FBOTargetParam::Colour0, 1);
    gBuffer.bindAttachment(FBOTargetParam::Colour1, 2);
    gBuffer.bindAttachment(FBOTargetParam::Colour2, 3);
    gBuffer.bindAttachment(FBOTargetParam::Colour3, 4);

    // Bind the downsampled depth.
    this->previousHiZPass->getInternalDataBlock<HiZPassDataBlock>()->hierarchicalDepth.bind(0);

    // Bind the camera block.
    rendererData->cameraBuffer.bindToPoint(0);

    // Bind the atmosphere data.
    this->previousSkyAtmoPass->getInternalDataBlock<SkyAtmospherePassDataBlock>()->atmosphereBuffer.bindToPoint(2);

    // Bind the BRDF LUT.
    this->passData.brdfLUT.bind(7);

    // Bind the HBAO texture.
    auto hbaoBlock = this->previousHBAOPass->getInternalDataBlock<HBAOPassDataBlock>();
    if (hbaoBlock->enableAO)
      hbaoBlock->ao.bind(8);

    // Bind the lighting buffer.
    rendererData->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::ReadWrite);

    // The IBL-specific parameters.
    glm::vec4 iblParams(0.0f, 0.0f, static_cast<float>(hbaoBlock->enableAO), 0.0f);

    //----------------------------------------------------------------------------
    // Dynamic sky IBL pass.
    //----------------------------------------------------------------------------
    // Bind the required cubemaps.
    auto dynIBLBlock = this->previousDynSkyIBLPass->getInternalDataBlock<DynamicSkyIBLPassDataBlock>();
    dynIBLBlock->radianceCubemaps.bind(6);
    dynIBLBlock->compactedDiffuseSH.bindToPoint(0u);

    // Bind the IBL parameters buffer.
    this->passData.iblBuffer.bindToPoint(1);

    bool launched = false;
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getWidth())
                                              / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(rendererData->lightingBuffer.getHeight())
                                               / 8.0f));
    for (auto& dynamicIBLParam : this->passData.dynamicIBLParams)
    {
      iblParams.x = static_cast<float>(dynamicIBLParam.handle);
      iblParams.y = dynamicIBLParam.intensity;
      iblParams.w = static_cast<float>(dynamicIBLParam.attachedSkyAtmoHandle);
      this->passData.iblBuffer.setData(0, sizeof(glm::vec4), &(iblParams.x));

      this->passData.iblEvaluation->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

      launched = true;
    }

    if (!launched)
    {
      this->passData.emissionEvaluation->launchCompute(iWidth, iHeight, 1);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }

    this->passData.dynamicIBLParams.clear();
    //----------------------------------------------------------------------------
  }

  void 
  IBLApplicationPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  IBLApplicationPass::onShutdown()
  { }

  void 
  IBLApplicationPass::submitDynamicSkyIBL(const DynamicIBL &params)
  {
    this->passData.dynamicIBLParams.push_back(params);
  }
}