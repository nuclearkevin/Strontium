#include "Graphics/RenderPasses/SkyAtmospherePass.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/GeometryPass.h"

namespace Strontium
{
  SkyAtmospherePass::SkyAtmospherePass(Renderer3D::GlobalRendererData* globalRendererData,
                                       GeometryPass* previousGeoPass)
    : RenderPass(&this->passData, globalRendererData, { previousGeoPass })
  {
    
  }

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
      this->passData.availableHandles.push(i);
  }

  void SkyAtmospherePass::updatePassData()
  { }

  RendererDataHandle 
  SkyAtmospherePass::requestRendererData()
  {
    if (!this->passData.availableHandles.empty())
    {
      auto top = this->passData.availableHandles.top();
      this->passData.availableHandles.pop();
      this->passData.activeHandles.emplace_back(top);
      return top;
    }

    return -1;
  }

  void 
  SkyAtmospherePass::deleteRendererData(const RendererDataHandle& handle)
  {
    auto& handleLoc = std::find(this->passData.activeHandles.begin(), 
                                this->passData.activeHandles.end(), handle);
    if (handleLoc != this->passData.activeHandles.end())
    {
      this->passData.activeHandles.erase(handleLoc);
      this->passData.availableHandles.push(handle);
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
    this->passData.updatableHandles.clear();
    this->passData.updatableHandles.reserve(this->passData.updateAtmosphere.count());
    for (auto& handle : this->passData.activeHandles)
    {
      if (this->passData.updateAtmosphere[handle])
      {
        this->passData.updatableHandles.emplace_back(handle);
        this->passData.updateAtmosphere[handle] = false;
      }
    }

    if (!(this->passData.updatableHandles.size() > 0u))
      return;

    auto start = std::chrono::steady_clock::now();

    // Send the atmosphere data to the GPU.
    this->passData.atmosphereBuffer.bindToPoint(0);
    this->passData.atmosphereBuffer.setData(0, MAX_NUM_ATMOSPHERES * sizeof(Atmosphere),
                                            this->passData.atmosphereQueue.data());
    this->passData.atmosphereIndices.bindToPoint(1);
    this->passData.atmosphereIndices.setData(0, this->passData.updatableHandles.size() * sizeof(int),
                                             this->passData.updatableHandles.data());

    // Update the mandatory LUTs.
    this->passData.transmittanceLUTs.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
    this->passData.transmittanceCompute->launchCompute(256 / 8, 64 / 8, this->passData.updatableHandles.size());
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    this->passData.multiscatterLUTS.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
    this->passData.transmittanceLUTs.bind(0);
    this->passData.multiScatteringCompute->launchCompute(1024, this->passData.updatableHandles.size(), 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    // Update the skyview LUTs if the atmosphere system uses the fast approach.
    if (this->passData.useFastAtmosphere)
    {
      this->passData.skyviewLUTs.bindAsImage(0, 0, true, 0, ImageAccessPolicy::Write);
      this->passData.multiscatterLUTS.bind(1);
      this->passData.skyviewCompute->launchCompute(256 / 8, 128 / 8, this->passData.updatableHandles.size());
      Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = end - start;
    this->passData.cpuTime = elapsed.count() * 1000.0f;
  }

  void SkyAtmospherePass::onRendererEnd(FrameBuffer& frontBuffer)
  {

  }

  void SkyAtmospherePass::onShutdown()
  {

  }
}