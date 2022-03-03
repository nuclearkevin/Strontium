#include "Graphics/RenderPasses/IBLApplicationPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  IBLApplicationPass::IBLApplicationPass(Renderer3D::GlobalRendererData* globalRendererData,
										 GeometryPass* previousGeoPass,
                                         HBAOPass* previousHBAOPass,
										 DynamicSkyIBLPass* previousDynSkyIBLPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousDynSkyIBLPass })
	, previousGeoPass(previousGeoPass)
    , previousHBAOPass(previousHBAOPass)
	, previousDynSkyIBLPass(previousDynSkyIBLPass)
	, timer(5)
  { }

  IBLApplicationPass::~IBLApplicationPass()
  { }

  void 
  IBLApplicationPass::onInit()
  {
    this->passData.iblEvaluation = ShaderCache::getShader("ibl_evaluation");
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

    auto geometryBlock = this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>();
    // Bind the GBuffer attachments.
    auto& gBuffer = geometryBlock->gBuffer;
    gBuffer.bindAttachment(FBOTargetParam::Depth, 0);
    gBuffer.bindAttachment(FBOTargetParam::Colour0, 1);
    gBuffer.bindAttachment(FBOTargetParam::Colour1, 2);
    gBuffer.bindAttachment(FBOTargetParam::Colour2, 3);

    // Bind the camera block.
    geometryBlock->cameraBuffer.bindToPoint(0);

    // Bind the BRDF LUT.
    this->passData.brdfLUT.bind(6);

    // Bind the HBAO texture.
    auto hbaoBlock = this->previousHBAOPass->getInternalDataBlock<HBAOPassDataBlock>();
    if (hbaoBlock->enableAO)
      hbaoBlock->downsampleAO.bind(7);

    // Bind the lighting buffer.
    this->globalBlock->lightingBuffer.bindAsImage(0, 0, ImageAccessPolicy::ReadWrite);

    // The IBL-specific parameters.
    glm::vec4 iblParams(0.0f, 0.0f, static_cast<float>(hbaoBlock->enableAO), 0.0f);

    //----------------------------------------------------------------------------
    // Dynamic sky IBL pass.
    //----------------------------------------------------------------------------
    // Bind the required cubemaps.
    auto dynIBLBlock = this->previousDynSkyIBLPass->getInternalDataBlock<DynamicSkyIBLPassDataBlock>();
    dynIBLBlock->irradianceCubemaps.bind(4);
    dynIBLBlock->radianceCubemaps.bind(5);

    // Bind the IBL parameters buffer.
    this->passData.iblBuffer.bindToPoint(1);

    for (auto& dynamicIBLParam : this->passData.dynamicIBLParams)
    {
      iblParams.x = static_cast<float>(dynamicIBLParam.handle);
      iblParams.y = dynamicIBLParam.intensity;
      this->passData.iblBuffer.setData(0, 3 * sizeof(float), &(iblParams.x));

      uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(globalBlock->lightingBuffer.getWidth())
                                                / 8.0f));
      uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(globalBlock->lightingBuffer.getHeight())
                                                 / 8.0f));
      this->passData.iblEvaluation->launchCompute(iWidth, iHeight, 1);
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
  IBLApplicationPass::submitDynamicSkyIBL(const DynamicIBL& params)
  {
    this->passData.dynamicIBLParams.push_back(params);
  }
}