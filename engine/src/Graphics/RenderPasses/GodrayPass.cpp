#include "Graphics/RenderPasses/GodrayPass.h"

#define Z_SLICES 256

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  GodrayPass::GodrayPass(Renderer3D::GlobalRendererData* globalRendererData,
                         GeometryPass* previousGeoPass, ShadowPass* previousShadowPass,
                         HiZPass* previousHiZPass, DirectionalLightPass* previousDirLightPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousShadowPass, previousHiZPass })
    , previousGeoPass(previousGeoPass)
    , previousShadowPass(previousShadowPass)
    , previousHiZPass(previousHiZPass)
    , previousDirLightPass(previousDirLightPass)
    , timer(5)
  { }

  GodrayPass::~GodrayPass()
  { }
  
  void 
  GodrayPass::onInit()
  {
    this->passData.godrayCompute = ShaderCache::getShader("screen_space_godrays");
    this->passData.godrayBlur = ShaderCache::getShader("screen_space_godrays_blur");

    Texture2DParams godrayParams = Texture2DParams();
    godrayParams.sWrap = TextureWrapParams::ClampEdges;
    godrayParams.tWrap = TextureWrapParams::ClampEdges;
    godrayParams.internal = TextureInternalFormats::RGBA16f;
    godrayParams.format = TextureFormats::RGBA;
    godrayParams.dataType = TextureDataType::Floats;
    this->passData.godrays.setSize(1600, 900);
    this->passData.godrays.setParams(godrayParams);
    this->passData.godrays.initNullTexture();

    Texture3DParams materialDataParams = Texture3DParams();
    materialDataParams.sWrap = TextureWrapParams::ClampEdges;
    materialDataParams.tWrap = TextureWrapParams::ClampEdges;
    materialDataParams.rWrap = TextureWrapParams::ClampEdges;
    materialDataParams.internal = TextureInternalFormats::RGBA16f;
    materialDataParams.format = TextureFormats::RGBA;
    materialDataParams.dataType = TextureDataType::Floats;

    glm::ivec2 size = static_cast<glm::ivec2>(glm::ceil(glm::vec2(1600.0f, 900.0f) / 8.0f));

    this->passData.scatExtinction.setSize(size.x, size.y, Z_SLICES);
    this->passData.scatExtinction.setParams(materialDataParams);
    this->passData.scatExtinction.initNullTexture();

    this->passData.emissionPhase.setSize(size.x, size.y, Z_SLICES);
    this->passData.emissionPhase.setParams(materialDataParams);
    this->passData.emissionPhase.initNullTexture();
  }

  void 
  GodrayPass::updatePassData()
  { }

  RendererDataHandle 
  GodrayPass::requestRendererData()
  {
    return -1;
  }

  void 
  GodrayPass::deleteRendererData(RendererDataHandle& handle)
  { }

  void 
  GodrayPass::onRendererBegin(uint width, uint height)
  {
    if (static_cast<uint>(this->passData.godrays.getWidth()) != width ||
        static_cast<uint>(this->passData.godrays.getHeight()) != height)
	{
	  this->passData.godrays.setSize(width, height);
	  this->passData.godrays.initNullTexture();
	}

    int fWidth = static_cast<int>(glm::ceil(static_cast<float>(width) / 8.0f));
    int fheight = static_cast<int>(glm::ceil(static_cast<float>(height) / 8.0f));
    if (this->passData.scatExtinction.getWidth() != fWidth ||
        this->passData.scatExtinction.getHeight() != fheight)
    {
      this->passData.scatExtinction.setSize(fWidth, fheight, Z_SLICES);
      this->passData.scatExtinction.initNullTexture();
      
      this->passData.emissionPhase.setSize(fWidth, fheight, Z_SLICES);
      this->passData.emissionPhase.initNullTexture();
    }

    this->passData.hasGodrays = false;

    // Clear the fog volumes.
    this->passData.obbVolumes.clear();
  }

  void 
  GodrayPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    auto dirLightBlock = this->previousDirLightPass->getInternalDataBlock<DirectionalLightPassDataBlock>();

    if (!(this->passData.enableGodrays && dirLightBlock->castShadows))
      return;
    this->passData.hasGodrays = true;

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);
    auto shadowBlock = this->previousShadowPass->getInternalDataBlock<ShadowPassDataBlock>();

    // Resize the SSBO for the fog volume data.
    this->passData.obbFogBuffer.resize(this->passData.obbVolumes.size() * sizeof(OBBFogVolume), BufferType::Dynamic);
    this->passData.obbFogBuffer.setData(0, this->passData.obbVolumes.size() * sizeof(OBBFogVolume), this->passData.obbVolumes.data());
    this->passData.obbFogBuffer.bindToPoint(0);

    // Bind the camera block.
    this->previousGeoPass->getInternalDataBlock<GeometryPassDataBlock>()
                         ->cameraBuffer.bindToPoint(0);

    struct GodrayBlockData
    {
      glm::vec4 mieScat; //  Mie scattering base (x, y, z) and density (w).
      glm::vec4 mieAbs; //  Mie absorption base (x, y, z) and density (w).
      glm::vec4 lightDirMiePhase; // Light direction (x, y, z) and the Mie phase (w).
      glm::vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
      glm::vec4 blurDirection;
      glm::ivec4 intData;
    }
      godrayData
    {
      { this->passData.mieScat },
      { this->passData.mieAbs },
      { glm::vec3(dirLightBlock->primaryLight.directionSize), this->passData.miePhase },
      { dirLightBlock->primaryLight.colourIntensity },
      { 1.0f, 0.0f, static_cast<float>(this->passData.numSteps), 0.0f },
      { this->passData.obbVolumes.size(), 0, 0, 0 }
    };

    // Populate and bind the godray parameters.
    this->passData.godrayParamsBuffer.setData(0, sizeof(GodrayBlockData), &godrayData);
    this->passData.godrayParamsBuffer.bindToPoint(1);

    // Populate the froxel material data.
    this->passData.scatExtinction.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
    this->passData.emissionPhase.bindAsImage(1, 0, true, 0, ImageAccessPolicy::Write);
    uint iFWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.scatExtinction.getWidth())
                                              / 8.0f));
    uint iFHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.scatExtinction.getHeight())
                                               / 8.0f));
    ShaderCache::getShader("populate_froxels")->launchCompute(iFWidth, iFHeight, Z_SLICES);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    // Bind the downsampled depth.
    auto hzBlock = this->previousHiZPass->getInternalDataBlock<HiZPassDataBlock>();
    hzBlock->hierarchicalDepth.bind(0);

    // Bind the noise texture.
    rendererData->spatialBlueNoise->bind(1);

    // Bind the shadow maps.
    for (uint i = 0; i < NUM_CASCADES; i++)
      shadowBlock->shadowBuffers[i].bindTextureID(FBOTargetParam::Depth, 4 + i);

    // Bind the shadow params buffer.
    dirLightBlock->cascadedShadowBlock.bindToPoint(2);

    // Bind the froxel material properties.
    this->passData.scatExtinction.bind(2);
    this->passData.emissionPhase.bind(3);

    // Compute the SSGR.
    rendererData->halfResBuffer1.bindAsImage(0, 0, ImageAccessPolicy::Write);
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(hzBlock->hierarchicalDepth.getWidth())
                                              / 16.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(hzBlock->hierarchicalDepth.getHeight())
                                               / 16.0f));
    this->passData.godrayCompute->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    // Blur the SSGR texture to get rid of noise from the jittering.
    rendererData->halfResBuffer1.bind(1);
    rendererData->fullResBuffer1.bindAsImage(0, 0, ImageAccessPolicy::Write);
    this->passData.godrayBlur->launchCompute(2 * iWidth, 2 * iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    
    godrayData.blurDirection.x = 0.0f;
    godrayData.blurDirection.y = 1.0f;
    this->passData.godrayParamsBuffer.setData(4 * sizeof(glm::vec4), sizeof(glm::vec2), &godrayData.blurDirection.x);
    rendererData->fullResBuffer1.bind(1);
    this->passData.godrays.bindAsImage(0, 0, ImageAccessPolicy::Write);
    this->passData.godrayBlur->launchCompute(2 * iWidth, 2 * iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
  }

  void 
  GodrayPass::onRendererEnd(FrameBuffer &frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  GodrayPass::onShutdown()
  { }

  void 
  GodrayPass::submit(const OBBFogVolume &fogVolume)
  {
    this->passData.obbVolumes.emplace_back(fogVolume);
  }
}