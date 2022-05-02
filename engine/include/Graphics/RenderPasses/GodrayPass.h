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
    Shader* godrayCompute;
    Shader* godrayBlur;

    Texture2D godrays;
    Texture3D scatExtinction;
    Texture3D emissionPhase;

    ShaderStorageBuffer obbFogBuffer;

    UniformBuffer godrayParamsBuffer;

    // Fog volumes.
    std::vector<OBBFogVolume> obbVolumes;

    bool enableGodrays;
    bool hasGodrays;
    uint numSteps;
    float miePhase;
    glm::vec4 mieScat; // Mie scattering coefficient (x, y, z) and density (w).
    glm::vec4 mieAbs; // Mie absorption coefficient (x, y, z) and density (w).

    // Some statistics to display.
    float frameTime;

    GodrayPassDataBlock()
      : godrayCompute(nullptr)
      , godrayBlur(nullptr)
      , obbFogBuffer(0, BufferType::Dynamic)
      , godrayParamsBuffer(6 *  sizeof(glm::vec4), BufferType::Dynamic)
      , enableGodrays(false)
      , hasGodrays(false)
      , numSteps(64u)
      , miePhase(0.8f)
      , mieScat(1.0f)
      , mieAbs(1.0f)
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