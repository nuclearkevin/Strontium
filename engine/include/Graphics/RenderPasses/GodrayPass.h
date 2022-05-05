#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/ShadowPass.h"
#include "Graphics/RenderPasses/HiZPass.h"
#include "Graphics/RenderPasses/DirectionalLightPass.h"

#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"
#include "Graphics/GPUTimers.h"

namespace Strontium
{
  namespace Renderer3D
  {
    struct GlobalRendererData;
  }

  struct GodrayPassDataBlock
  {
    Texture3D scatExtinction;
    Texture3D emissionPhase;
    Texture3D lightExtinction;
    Texture3D finalGather;

    ShaderStorageBuffer obbFogBuffer;

    UniformBuffer godrayParamsBuffer;

    // Fog volumes.
    std::vector<OBBFogVolume> obbVolumes;

    bool enableGodrays;
    bool hasGodrays;
    uint numZSlices;
    glm::uvec2 bufferSize;

    // Some statistics to display.
    float frameTime;

    GodrayPassDataBlock()
      : obbFogBuffer(0, BufferType::Dynamic)
      , godrayParamsBuffer(3 * sizeof(glm::ivec4), BufferType::Dynamic)
      , enableGodrays(false)
      , hasGodrays(false)
      , numZSlices(512u)
      , bufferSize(1u, 1u)
      , frameTime(0.0f)
    { }
  };

  class GodrayPass final : public RenderPass
  {
  public:
    GodrayPass(Renderer3D::GlobalRendererData* globalRendererData,
               GeometryPass* previousGeoPass, ShadowPass* previousShadowPass,
               HiZPass* previousHiZPass, DirectionalLightPass* previousDirLightPass);
    ~GodrayPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;

    // Submit fog volumes.
    void submit(const OBBFogVolume &fogVolume);

  private:
    GodrayPassDataBlock passData;

    GeometryPass* previousGeoPass;
    ShadowPass* previousShadowPass;
    HiZPass* previousHiZPass;
    DirectionalLightPass* previousDirLightPass;

    AsynchTimer timer;
  };
}