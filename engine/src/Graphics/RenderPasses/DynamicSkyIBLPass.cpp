#include "Graphics/RenderPasses/DynamicSkyIBLPass.h"

// Project includes.
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
  {

  }

  void 
  DynamicSkyIBLPass::onInit()
  {
    this->passData.dynamicSkyIrradiance = ShaderCache::getShader("sky_lut_diffuse");
    this->passData.dynamicSkyRadiance = ShaderCache::getShader("sky_lut_specular");

    // Common texture params.
    TextureCubeMapParams params = TextureCubeMapParams();
    params.internal = TextureInternalFormats::RGBA16f;
    params.dataType = TextureDataType::Floats;

    // Initialize the irradiance (diffuse) cubemap texture array for bulk computation.
    this->passData.irradianceCubemaps.setSize(32, 32, 8);
    this->passData.irradianceCubemaps.setParams(params);
    this->passData.irradianceCubemaps.initNullTexture();

    // Initialize the radiance (specular) cubemap texture array for bulk computation.
    params.minFilter = TextureMinFilterParams::LinearMipMapLinear;
    this->passData.radianceCubemaps.setSize(64, 64, 8);
    this->passData.radianceCubemaps.setParams(params);
    this->passData.radianceCubemaps.initNullTexture();

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
  DynamicSkyIBLPass::submit(const DynamicIBL& iblParams, RendererDataHandle handle)
  {
    if (iblParams.attachedSkyAtmoHandle != this->passData.iblQueue[handle].attachedSkyAtmoHandle)
    {
      this->passData.iblQueue[handle] = iblParams;
      this->passData.updateIBL[handle] = true;
    }
  }
}