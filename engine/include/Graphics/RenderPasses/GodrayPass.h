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
    Texture3D historyResolve[2];
    Texture3D finalGather;

    bool taaVolume;
    bool resolveFlag;

    bool enableGodrays;
    bool hasGodrays;
    uint numZSlices;
    glm::uvec2 bufferSize;

    ShaderStorageBuffer obbFogBuffer;
    ShaderStorageBuffer sphereFogBuffer;

    UniformBuffer godrayParamsBuffer;

    // Depth fog parameters.
    bool applyDepthFog;
    float minDepthDensity;
    float maxDepthDensity;
    glm::vec4 mieScatteringPhaseDepth;
    glm::vec4 emissionAbsorptionDepth;

    // Height fog parameters.
    bool applyHeightFog;
    float heightFalloff;
    float heightDensity;
    glm::vec4 mieScatteringPhaseHeight;
    glm::vec4 emissionAbsorptionHeight;

    // Quick and dirty flat ambient.
    glm::vec3 ambientColour;
    float ambientIntensity;

    // Fog volumes.
    std::vector<OBBFogVolume> obbVolumes;
    std::vector<SphereFogVolume> sphereVolumes;

    // Some statistics to display.
    float frameTime;

    GodrayPassDataBlock()
      : obbFogBuffer(0, BufferType::Dynamic)
      , sphereFogBuffer(0, BufferType::Dynamic)
      , godrayParamsBuffer(10 * sizeof(glm::vec4), BufferType::Dynamic)
      , resolveFlag(false)
      , taaVolume(true)
      , enableGodrays(false)
      , hasGodrays(false)
      , numZSlices(128u)
      , bufferSize(1u, 1u)
      , applyDepthFog(false)
      , minDepthDensity(0.0f)
      , maxDepthDensity(0.1f)
      , mieScatteringPhaseDepth(0.1f, 0.1f, 0.1f, 0.8f)
      , emissionAbsorptionDepth(0.0f, 0.0f, 0.0f, 1.0f)
      , applyHeightFog(false)
      , heightFalloff(1.0f)
      , heightDensity(0.1f)
      , mieScatteringPhaseHeight(0.1f, 0.1f, 0.1f, 0.8f)
      , emissionAbsorptionHeight(0.0f, 0.0f, 0.0f, 1.0f)
      , ambientColour(0.5294f, 0.8078f, 0.9216f)
      , ambientIntensity(1.0f)
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
    void submit(const SphereFogVolume& fogVolume);

  private:
    GodrayPassDataBlock passData;

    GeometryPass* previousGeoPass;
    ShadowPass* previousShadowPass;
    HiZPass* previousHiZPass;
    DirectionalLightPass* previousDirLightPass;

    AsynchTimer timer;
  };
}