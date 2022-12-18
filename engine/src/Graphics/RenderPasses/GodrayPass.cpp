#include "Graphics/RenderPasses/GodrayPass.h"

// Project includes.
#include "Graphics/Renderer.h"

namespace Strontium
{
  GodrayPass::GodrayPass(Renderer3D::GlobalRendererData* globalRendererData,
                         GeometryPass* previousGeoPass, ShadowPass* previousShadowPass,
                         HiZPass* previousHiZPass, DirectionalLightPass* previousDirLightPass, 
                         AreaLightPass* previousAreaLightPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass, previousShadowPass, previousHiZPass, 
                                                        previousDirLightPass, previousAreaLightPass })
    , previousGeoPass(previousGeoPass)
    , previousShadowPass(previousShadowPass)
    , previousHiZPass(previousHiZPass)
    , previousDirLightPass(previousDirLightPass)
    , previousAreaLightPass(previousAreaLightPass)
    , timer(5)
  { }

  GodrayPass::~GodrayPass()
  { }
  
  void 
  GodrayPass::onInit()
  {
    Texture3DParams froxelParams = Texture3DParams();
    froxelParams.sWrap = TextureWrapParams::ClampEdges;
    froxelParams.tWrap = TextureWrapParams::ClampEdges;
    froxelParams.rWrap = TextureWrapParams::ClampEdges;
    froxelParams.internal = TextureInternalFormats::RGBA16f;
    froxelParams.format = TextureFormats::RGBA;
    froxelParams.dataType = TextureDataType::Floats;

    this->passData.bufferSize = static_cast<glm::ivec2>(glm::ceil(glm::vec2(1600.0f, 900.0f) / 8.0f));

    this->passData.scatExtinction.setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.scatExtinction.setParams(froxelParams);
    this->passData.scatExtinction.initNullTexture();

    this->passData.emissionPhase.setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.emissionPhase.setParams(froxelParams);
    this->passData.emissionPhase.initNullTexture();

    this->passData.lightExtinction.setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.lightExtinction.setParams(froxelParams);
    this->passData.lightExtinction.initNullTexture();

    this->passData.historyResolve[0].setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.historyResolve[0].setParams(froxelParams);
    this->passData.historyResolve[0].initNullTexture();
    this->passData.historyResolve[1].setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.historyResolve[1].setParams(froxelParams);
    this->passData.historyResolve[1].initNullTexture();

    this->passData.finalGather.setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.finalGather.setParams(froxelParams);
    this->passData.finalGather.initNullTexture();
  }

  void 
  GodrayPass::updatePassData()
  {
    this->passData.scatExtinction.setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.scatExtinction.initNullTexture();
    
    this->passData.emissionPhase.setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.emissionPhase.initNullTexture();

    this->passData.lightExtinction.setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.lightExtinction.initNullTexture();

    this->passData.historyResolve[0].setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.historyResolve[0].initNullTexture();
    this->passData.historyResolve[1].setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.historyResolve[1].initNullTexture();

    this->passData.finalGather.setSize(this->passData.bufferSize.x, this->passData.bufferSize.y, this->passData.numZSlices);
    this->passData.finalGather.initNullTexture();
  }

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
    int fWidth = static_cast<int>(glm::ceil(static_cast<float>(width) / 8.0f));
    int fheight = static_cast<int>(glm::ceil(static_cast<float>(height) / 8.0f));
    if (this->passData.scatExtinction.getWidth() != fWidth ||
        this->passData.scatExtinction.getHeight() != fheight)
    {
      this->passData.bufferSize = glm::uvec2(fWidth, fheight);
      this->passData.scatExtinction.setSize(fWidth, fheight, this->passData.numZSlices);
      this->passData.scatExtinction.initNullTexture();
      
      this->passData.emissionPhase.setSize(fWidth, fheight, this->passData.numZSlices);
      this->passData.emissionPhase.initNullTexture();

      this->passData.lightExtinction.setSize(fWidth, fheight, this->passData.numZSlices);
      this->passData.lightExtinction.initNullTexture();

      this->passData.historyResolve[0].setSize(fWidth, fheight, this->passData.numZSlices);
      this->passData.historyResolve[0].initNullTexture();
      this->passData.historyResolve[1].setSize(fWidth, fheight, this->passData.numZSlices);
      this->passData.historyResolve[1].initNullTexture();

      this->passData.finalGather.setSize(fWidth, fheight, this->passData.numZSlices);
      this->passData.finalGather.initNullTexture();
    }

    // Clear the fog volumes.
    this->passData.obbVolumes.clear();
    this->passData.sphereVolumes.clear();

    this->passData.hasGodrays = false;
  }

  void 
  GodrayPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    auto dirLightBlock = this->previousDirLightPass->getInternalDataBlock<DirectionalLightPassDataBlock>();
    auto areaLightBlock = this->previousAreaLightPass->getInternalDataBlock<AreaLightPassDataBlock>();

    if (!(this->passData.enableGodrays && dirLightBlock->hasPrimary))
      return;

    this->passData.hasGodrays = true;

    auto rendererData = static_cast<Renderer3D::GlobalRendererData*>(this->globalBlock);
    auto shadowBlock = this->previousShadowPass->getInternalDataBlock<ShadowPassDataBlock>();

    // Resize the SSBOs for the fog volume data.
    this->passData.obbFogBuffer.resize(this->passData.obbVolumes.size() * sizeof(OBBFogVolume), BufferType::Dynamic);
    this->passData.obbFogBuffer.setData(0, this->passData.obbVolumes.size() * sizeof(OBBFogVolume), this->passData.obbVolumes.data());
    this->passData.obbFogBuffer.bindToPoint(0);

    this->passData.sphereFogBuffer.resize(this->passData.sphereVolumes.size() * sizeof(SphereFogVolume), BufferType::Dynamic);
    this->passData.sphereFogBuffer.setData(0, this->passData.sphereVolumes.size() * sizeof(SphereFogVolume), this->passData.sphereVolumes.data());
    this->passData.sphereFogBuffer.bindToPoint(1);

    // Bind the camera block.
    rendererData->cameraBuffer.bindToPoint(0);

    struct GodrayBlockData
    {
      glm::vec4 mieScatteringPhaseDepth; // Mie scattering (x, y, z) and phase value (w).
      glm::vec4 emissionAbsorptionDepth; // Emission (x, y, z) and absorption (w).
      glm::vec4 minMaxDensity; // Minimum density (x) and maximum density (y). z and w are unused.
      glm::vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
      glm::vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
      glm::vec4 falloff; // Falloff (x). y, z and w are unused.
      glm::vec4 lightDir; // Light direction (x, y, z). w is unused.
      glm::vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
      glm::vec4 ambientColourIntensity; // Ambient colour (x, y, z) and intensity (w).
      glm::ivec4 intData;
    }
      godrayData
    {
      { this->passData.mieScatteringPhaseDepth },
      { this->passData.emissionAbsorptionDepth },
      { this->passData.minDepthDensity, this->passData.maxDepthDensity, 0.0f, 0.0f },
      { glm::vec3(this->passData.mieScatteringPhaseHeight * this->passData.heightDensity), 
        this->passData.mieScatteringPhaseHeight.w },
      { this->passData.emissionAbsorptionHeight * this->passData.heightDensity },
      { this->passData.heightFalloff, 0.0f, 0.0f, 0.0f },
      { glm::vec3(dirLightBlock->primaryLight.directionSize), 0.0f },
      { dirLightBlock->primaryLight.colourIntensity },
      { this->passData.ambientColour, this->passData.ambientIntensity },
      { this->passData.obbVolumes.size(), 0, this->passData.sphereVolumes.size(), 0 }
    };

    // Bitflags. Bit 1 is if a global depth-based fog should be applied. 
    // Bit 2 is if a global height-based fog should be applied.
    // Bit 3 is if the light is shadowed or not.
    if (this->passData.applyDepthFog)
      godrayData.intData.y |= (1 << 0);
    if (this->passData.applyHeightFog)
      godrayData.intData.y |= (1 << 1);
    if (dirLightBlock->castShadows)
      godrayData.intData.y |= (1 << 2);

    // Bind the noise texture.
    rendererData->temporalBlueNoise->bind(2);

    // Populate and bind the godray parameters.
    this->passData.godrayParamsBuffer.setData(0, sizeof(GodrayBlockData), &godrayData);
    this->passData.godrayParamsBuffer.bindToPoint(1);

    // Bind the temporal AA block.
    rendererData->temporalBuffer.bindToPoint(3);

    // Populate the froxel material data.
    this->passData.scatExtinction.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
    this->passData.emissionPhase.bindAsImage(1, 0, true, 0, ImageAccessPolicy::Write);
    uint iFWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.scatExtinction.getWidth())
                                              / 8.0f));
    uint iFHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->passData.scatExtinction.getHeight())
                                               / 8.0f));
    ShaderCache::getShader("populate_froxels")->launchCompute(iFWidth, iFHeight, this->passData.numZSlices);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    // Bind the froxel material properties.
    this->passData.scatExtinction.bind(0);
    this->passData.emissionPhase.bind(1);

    // Bind directional light shadow maps and parameters.
    if (dirLightBlock->castShadows)
    {
      for (uint i = 0; i < NUM_CASCADES; i++)
        shadowBlock->shadowBuffers[i].bindTextureID(FBOTargetParam::Depth, 3 + i);
      dirLightBlock->cascadedShadowBlock.bindToPoint(2);
    }

    // Light the froxels for directional and ambient light.
    this->passData.lightExtinction.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
    if (dirLightBlock->castShadows)
      ShaderCache::getShader("light_froxels_shadowed")->launchCompute(iFWidth, iFHeight, this->passData.numZSlices);
    else
      ShaderCache::getShader("light_froxels")->launchCompute(iFWidth, iFHeight, this->passData.numZSlices);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    // Light the froxels for area lights.
    if (areaLightBlock->rectAreaLightCount > 0u)
    {
      areaLightBlock->ltcLUT2.bind(3);
      areaLightBlock->lightBlock.bindToPoint(1);
      ShaderCache::getShader("light_froxels_area")->launchCompute(iFWidth, iFHeight, this->passData.numZSlices);
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }

    // Temporally AA the froxel lighting buffer.
    this->passData.lightExtinction.bind(0);
    if (this->passData.taaVolume)
    {
      if (this->passData.resolveFlag)
      {
        this->passData.historyResolve[0].bind(1);
        this->passData.historyResolve[1].bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
        ShaderCache::getShader("taa_froxels")->launchCompute(iFWidth, iFHeight, this->passData.numZSlices);
        Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
      
        // Bind the lighting texture. 
        this->passData.historyResolve[1].bind(0);
      }
      else
      {
        this->passData.historyResolve[1].bind(1);
        this->passData.historyResolve[0].bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
        ShaderCache::getShader("taa_froxels")->launchCompute(iFWidth, iFHeight, this->passData.numZSlices);
        Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
      
        // Bind the lighting texture. 
        this->passData.historyResolve[0].bind(0);
      }
      this->passData.resolveFlag = !this->passData.resolveFlag;
    }

    // Perform the final gather.
    this->passData.finalGather.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
    ShaderCache::getShader("gather_froxels")->launchCompute(iFWidth, iFHeight, 1);
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

  void 
  GodrayPass::submit(const SphereFogVolume& fogVolume)
  {
    this->passData.sphereVolumes.emplace_back(fogVolume);
  }
}