#pragma once

#define MAX_NUM_ATMOSPHERES 8

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"

#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  class GeometryPass;

  struct SkyAtmospherePassDataBlock
  {
    Shader* transmittanceCompute;
    Shader* multiScatteringCompute;
    Shader* skyviewCompute;

    Texture2DArray transmittanceLUTs;
    Texture2DArray multiscatterLUTS;
    Texture2DArray skyviewLUTs;

    Texture2D halfResRayMarchBuffer;

    UniformBuffer atmosphereBuffer;
    ShaderStorageBuffer atmosphereIndices;

    // The atmospheres.
    std::bitset<MAX_NUM_ATMOSPHERES> updateAtmosphere;
    std::array<Atmosphere, MAX_NUM_ATMOSPHERES> atmosphereQueue;

    // Stacks for the inactive and active atmospheres.
    std::stack<RendererDataHandle> availableHandles;
    std::vector<RendererDataHandle> activeHandles;
    std::vector<RendererDataHandle> updatableHandles;

    // Use the skyview and aerial perspective LUTS instead of half-resolution raymarching.
    bool useFastAtmosphere;

    // Some statistics to display.
    float cpuTime;

    SkyAtmospherePassDataBlock()
      : transmittanceCompute(nullptr)
      , multiScatteringCompute(nullptr)
      , skyviewCompute(nullptr)
      , atmosphereBuffer(8 * sizeof(Atmosphere), BufferType::Dynamic)
      , atmosphereIndices(8 * sizeof(int), BufferType::Dynamic)
      , atmosphereQueue()
      , useFastAtmosphere(true)
      , cpuTime(0.0f)
    { }
  };

  class SkyAtmospherePass : public RenderPass
  {
  public:
    SkyAtmospherePass(Renderer3D::GlobalRendererData* globalRendererData,
                      GeometryPass* previousGeoPass);
    ~SkyAtmospherePass();

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(const RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;
  private:
    SkyAtmospherePassDataBlock passData;
  };
}