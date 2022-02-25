#pragma once

#define MAX_NUM_DYNAMIC_IBL 8

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/GPUTimers.h"
#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  class SkyAtmospherePass;

  struct DynamicSkyIBLPassDataBlock
  {
    Shader* dynamicSkyIrradiance;
    Shader* dynamicSkyRadiance;

    CubeMapArrayTexture irradianceCubemaps;
    CubeMapArrayTexture radianceCubemaps;

    // The IBL maps.
    std::bitset<MAX_NUM_DYNAMIC_IBL> updateIBL;
    std::array<DynamicIBL, MAX_NUM_DYNAMIC_IBL> iblQueue;

    // Some statistics.
    float frameTime;

    DynamicSkyIBLPassDataBlock()
      : dynamicSkyIrradiance(nullptr)
      , dynamicSkyRadiance(nullptr)
      , frameTime(0.0f)
    { }
  };

  class DynamicSkyIBLPass : public RenderPass
  {
  public:
	DynamicSkyIBLPass(Renderer3D::GlobalRendererData* globalRendererData, 
                      SkyAtmospherePass* previousSkyAtmoPass);
	~DynamicSkyIBLPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;

    void submit(const DynamicIBL &iblParams, RendererDataHandle handle);

  private:
    DynamicSkyIBLPassDataBlock passData;

    // Stacks for the inactive and active IBL components.
    std::stack<RendererDataHandle> availableHandles;
    std::vector<RendererDataHandle> activeHandles;
    std::vector<RendererDataHandle> updatableHandles;

    SkyAtmospherePass* previousSkyAtmoPass;

    AsynchTimer timer;
  };
}