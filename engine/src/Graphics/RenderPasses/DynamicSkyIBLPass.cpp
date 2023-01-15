#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"

namespace Strontium
{
  DynamicSkyIBLPass::DynamicSkyIBLPass(Renderer3D::GlobalRendererData* globalRendererData, 
                                       SkyAtmospherePass* previousSkyAtmoPass)
    : RenderPass(&this->passData, globalRendererData, { previousSkyAtmoPass })
    , previousSkyAtmoPass(previousSkyAtmoPass)
    , timer(5)
  { }

  DynamicSkyIBLPass::~DynamicSkyIBLPass()
  { }

  void 
  DynamicSkyIBLPass::onInit()
  {
    this->passData.shDynamicSkyIrradiance = ShaderCache::getShader("sky_lut_diffuse_sh");
    this->passData.shReduce = ShaderCache::getShader("sh_reduction");
    this->passData.shCompact = ShaderCache::getShader("sh_compaction");
    this->passData.dynamicSkyRadiance = ShaderCache::getShader("sky_lut_specular");

    // Common texture params.
    TextureCubeMapParams params = TextureCubeMapParams();
    params.internal = TextureInternalFormats::RGBA16f;
    params.dataType = TextureDataType::Floats;

    // Initialize the radiance (specular) cubemap texture array for bulk computation.
    params.minFilter = TextureMinFilterParams::LinearMipMapLinear;
    this->passData.radianceCubemaps.setSize(64, 64, 8);
    this->passData.radianceCubemaps.setParams(params);
    this->passData.radianceCubemaps.initNullTexture();
    this->passData.radianceCubemaps.generateMips();
    this->passData.radianceCubemaps.setParams(params);

    // Fill the stack with available atmosphere slots.
    for (RendererDataHandle i = 7; i > -1; i--)
      this->availableHandles.push(i);
  }

  void 
  DynamicSkyIBLPass::updatePassData()
  {

  }

  RendererDataHandle 
  DynamicSkyIBLPass::requestRendererData()
  {
    if (!this->availableHandles.empty())
    {
      auto top = this->availableHandles.top();
      this->availableHandles.pop();
      this->activeHandles.emplace_back(top);

      return top;
    }

    return -1;
  }

  void 
  DynamicSkyIBLPass::deleteRendererData(RendererDataHandle& handle)
  {
    if (handle < 0)
      return;

    auto& handleLoc = std::find(this->activeHandles.begin(), 
                                this->activeHandles.end(), handle);
    if (handleLoc != this->activeHandles.end())
    {
      this->activeHandles.erase(handleLoc);
      this->availableHandles.push(handle);

      handle = -1;
    }
  }

  void 
  DynamicSkyIBLPass::onRendererBegin(uint width, uint height)
  { }

  void 
  DynamicSkyIBLPass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    this->updatableHandles.clear();
    this->updatedSkyHandles.clear();
    this->updatableHandles.reserve(this->passData.updateIBL.count());
    this->updatedSkyHandles.reserve(this->passData.updateIBL.count());
    for (auto& handle : this->activeHandles)
    {
      // Error handle if the sky-atmosphere handle isn't valid.
      if (this->passData.updateIBL[handle] 
          && this->passData.iblQueue[handle].attachedSkyAtmoHandle >= 0)
      {
        this->updatableHandles.emplace_back(handle);
        this->updatedSkyHandles.emplace_back(this->passData.iblQueue[handle].attachedSkyAtmoHandle);
        this->passData.updateIBL[handle] = false;
      }
      else
        this->passData.updateIBL[handle] = false;
    }

    if (!(this->updatableHandles.size() > 0u))
      return;

    auto skyData = this->previousSkyAtmoPass->getInternalDataBlock<SkyAtmospherePassDataBlock>();

    // Bind the atmosphere data.
    skyData->atmosphereBuffer.bindToPoint(0);

    // Send over the updatable atmosphere and IBL handles.
    // TODO: Better batching scheme?
    this->passData.iblIndices.bindToPoint(0);
    this->passData.iblIndices.setData(0, sizeof(int) * this->updatedSkyHandles.size(), this->updatedSkyHandles.data());
    this->passData.iblIndices.setData(8 * sizeof(int), sizeof(int) * this->updatableHandles.size(), this->updatableHandles.data());

    // Sky-view LUTs.
    skyData->skyviewLUTs.bind(0);

    // Irradiance SH pass. 
    // SH params.
    this->passData.diffuseSHParams.bindToPoint(1);
    glm::ivec4 data(512, 512, this->updatableHandles.size(), 0);
    this->passData.diffuseSHParams.setData(0u, sizeof(glm::ivec4), &(data.x));
    
    // Storage for all the coefficients.
    this->passData.diffuseSHStorage.bindToPoint(1);

    // First pass to generate spherical harmonics components.
    this->passData.shDynamicSkyIrradiance->launchCompute(32, 16, this->updatableHandles.size());
    Shader::memoryBarrier(MemoryBarrierType::ShaderStorageBufferWrites);

    // Second pass to perform a somewhat parallel reduction.
    this->passData.shReduce->launchCompute(1, 1, this->updatableHandles.size());
    Shader::memoryBarrier(MemoryBarrierType::ShaderStorageBufferWrites);

    // Final pass to normalize and compact the SH coefficients.
    this->passData.compactedDiffuseSH.bindToPoint(2);
    this->passData.shCompact->launchCompute(static_cast<uint>(glm::ceil(static_cast<float>(this->updatableHandles.size()) / 64.0f)), 1, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderStorageBufferWrites);
    
    // Send over IBL parameters.
    this->passData.iblParamsBuffer.bindToPoint(1);
    struct IBLParamData
    {
      glm::vec4 params;
    }
    iblParamBlock
    {
      { 0.0f, 0.0f, static_cast<float>(this->passData.numRadSamples), 0.0f }
    };
    this->passData.iblParamsBuffer.setData(0, sizeof(IBLParamData), &iblParamBlock);

    // Radiance pass. This is more complicated...
    for (uint i = 0; i < 7; i++)
    {
      // Bind the storage mip.
      this->passData.radianceCubemaps.bindAsImage(0, i, true, 0, ImageAccessPolicy::Write);

      // Roughness.
      iblParamBlock.params.x = static_cast<float>(i) / 4.0f;
      this->passData.iblParamsBuffer.setData(0, sizeof(float), &(iblParamBlock.params.x));

      // Launch groups.
      uint mipWidthHeight = static_cast<uint>(64.0f * std::pow(0.5f, i));
      uint groupXY = static_cast<uint>(glm::ceil(static_cast<float>(mipWidthHeight) / 8.0f));
      this->passData.dynamicSkyRadiance->launchCompute(groupXY, groupXY, 6 * this->updatableHandles.size());
    }

    // This should work since there are no dependancies between the compute passes.
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
  }

  void 
  DynamicSkyIBLPass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void 
  DynamicSkyIBLPass::onShutdown()
  {

  }

  void 
  DynamicSkyIBLPass::submit(const DynamicIBL& iblParams, bool skyUpdated)
  {
    if (this->passData.iblQueue[iblParams.handle].attachedSkyAtmoHandle != iblParams.attachedSkyAtmoHandle || skyUpdated)
    {
      this->passData.iblQueue[iblParams.handle] = iblParams;
      this->passData.updateIBL[iblParams.handle] = true;
    }
  }
}