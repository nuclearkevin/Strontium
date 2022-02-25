#include "Graphics/RenderPasses/SkyAtmospherePass.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/GeometryPass.h"

namespace Strontium
{
  SkyAtmospherePass::SkyAtmospherePass(Renderer3D::GlobalRendererData* globalRendererData,
                                       GeometryPass* previousGeoPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass })
    , timer(5)
  { }

  SkyAtmospherePass::~SkyAtmospherePass()
  { }
  
  void SkyAtmospherePass::onInit()
  {
    this->passData.transmittanceCompute = ShaderCache::getShader("hillaire_transmittance");
    this->passData.multiScatteringCompute = ShaderCache::getShader("hillaire_multiscat");
    this->passData.skyviewCompute = ShaderCache::getShader("hillaire_skyview");

    Texture2DParams params = Texture2DParams();
    params.sWrap = TextureWrapParams::ClampEdges;
    params.tWrap = TextureWrapParams::ClampEdges;
    params.internal = TextureInternalFormats::RGBA16f;
    params.format = TextureFormats::RGBA;
    params.dataType = TextureDataType::Floats;

    this->passData.transmittanceLUTs.setSize(256, 64, 8);
    this->passData.transmittanceLUTs.setParams(params);
    this->passData.transmittanceLUTs.initNullTexture();

    this->passData.multiscatterLUTS.setSize(32, 32, 8);
    this->passData.multiscatterLUTS.setParams(params);
    this->passData.multiscatterLUTS.initNullTexture();

    this->passData.skyviewLUTs.setSize(256, 128, 8);
    this->passData.skyviewLUTs.setParams(params);
    this->passData.skyviewLUTs.initNullTexture();

    this->passData.halfResRayMarchBuffer.setSize(1600 / 2, 900 / 2);
    this->passData.halfResRayMarchBuffer.setParams(params);
    this->passData.halfResRayMarchBuffer.initNullTexture();

    // Fill the stack with available atmosphere slots.
    for (RendererDataHandle i = 7; i > -1; i--)
      this->availableHandles.push(i);
  }

  void SkyAtmospherePass::updatePassData()
  { }

  RendererDataHandle 
  SkyAtmospherePass::requestRendererData()
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
  SkyAtmospherePass::deleteRendererData(RendererDataHandle& handle)
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

  void SkyAtmospherePass::onRendererBegin(uint width, uint height)
  {
    uint texWidth = static_cast<uint>(this->passData.halfResRayMarchBuffer.getWidth());
	uint texheight = static_cast<uint>(this->passData.halfResRayMarchBuffer.getHeight());

    uint hWidth = static_cast<uint>(glm::ceil(static_cast<float>(width) / 2.0f));
    uint hHeight = static_cast<uint>(glm::ceil(static_cast<float>(height) / 2.0f));

    if (!this->passData.useFastAtmosphere && 
        (texWidth != hWidth || texheight != hHeight))
    {
      this->passData.halfResRayMarchBuffer.setSize(hWidth, hHeight);
      this->passData.halfResRayMarchBuffer.initNullTexture();
    }
  }

  void SkyAtmospherePass::onRender()
  {
    ScopedTimer<AsynchTimer> profiler(this->timer);

    this->updatableHandles.clear();
    this->updatableHandles.reserve(this->passData.updateAtmosphere.count());
    for (auto& handle : this->activeHandles)
    {
      if (this->passData.updateAtmosphere[handle])
      {
        this->updatableHandles.emplace_back(handle);
        this->passData.updateAtmosphere[handle] = false;
      }
    }
    
    if (!(this->updatableHandles.size() > 0u))
      return;

    // Send the atmosphere data to the GPU.
    this->passData.atmosphereBuffer.bindToPoint(0);
    this->passData.atmosphereBuffer.setData(0, MAX_NUM_ATMOSPHERES * sizeof(Atmosphere),
                                            this->passData.atmosphereQueue.data());
    this->passData.atmosphereIndices.bindToPoint(1);
    this->passData.atmosphereIndices.setData(0, this->updatableHandles.size() * sizeof(int),
                                             this->updatableHandles.data());

    // Update the mandatory LUTs.
    this->passData.transmittanceLUTs.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
    this->passData.transmittanceCompute->launchCompute(256 / 8, 64 / 8, this->updatableHandles.size());
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    this->passData.multiscatterLUTS.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
    this->passData.transmittanceLUTs.bind(0);
    this->passData.multiScatteringCompute->launchCompute(1024, this->updatableHandles.size(), 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    // Update the skyview LUTs if the atmosphere system uses the fast approach.
    if (this->passData.useFastAtmosphere)
    {
      this->passData.skyviewLUTs.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
      this->passData.multiscatterLUTS.bind(1);
      this->passData.skyviewCompute->launchCompute(256 / 8, 128 / 8, this->updatableHandles.size());
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }
  }

  void SkyAtmospherePass::onRendererEnd(FrameBuffer& frontBuffer)
  {
    this->timer.msRecordTime(this->passData.frameTime);
  }

  void SkyAtmospherePass::onShutdown()
  {

  }

  void 
  SkyAtmospherePass::submit(const Atmosphere& atmo, const RendererDataHandle& handle, 
                            const DirectionalLight &light, const glm::mat4& model)
  {
    Atmosphere temp = atmo;
    
    // TODO: Camera position.
    temp.viewPos = glm::vec4(0.0f, 6.360f + 0.0002f, 0.0f, 0.0f);

    // Fetch the light direction from the light attached 
    // with the accompanying entity.
    auto invTrans = glm::transpose(glm::inverse(model));
    auto dir = glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
    temp.sunDirAtmRadius = glm::vec4(dir, atmo.sunDirAtmRadius.w);
    temp.lightColourIntensity = light.colourIntensity;

    if (temp != this->passData.atmosphereQueue[handle])
    {
      this->passData.atmosphereQueue[handle] = temp;
      this->passData.updateAtmosphere[handle] = true;
    }
  }

  void 
  SkyAtmospherePass::submit(const Atmosphere& atmo, const RendererDataHandle& handle, 
                            const glm::mat4& model)
  {
    Atmosphere temp = atmo;
    
    // TODO: Camera position.
    temp.viewPos = glm::vec4(0.0f, 6.360f + 0.0002f, 0.0f, 0.0f);

    // Fetch the light direction from either the primary light or the 
    // light attached with the accompanying entity.
    auto invTrans = glm::transpose(glm::inverse(model));
    if (this->globalBlock->primaryLightIndex > -1)
    {
      auto dir = glm::vec3(-this->globalBlock->directionalLightQueue[this->globalBlock->primaryLightIndex].direction);
      temp.sunDirAtmRadius = glm::vec4(dir, atmo.sunDirAtmRadius.w);
      temp.lightColourIntensity = this->globalBlock->directionalLightQueue[this->globalBlock->primaryLightIndex].colourIntensity;
    }
    else
    {
      auto dir = glm::vec3(invTrans * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
      temp.sunDirAtmRadius = glm::vec4(dir, atmo.sunDirAtmRadius.w);
      temp.lightColourIntensity = glm::vec4(1.0f);
    }

    if (temp != this->passData.atmosphereQueue[handle])
    {
      this->passData.atmosphereQueue[handle] = temp;
      this->passData.updateAtmosphere[handle] = true;
    }
  }
}