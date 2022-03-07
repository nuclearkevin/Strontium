#pragma once

// Technically MAX_NUM_DIRECTIONAL_LIGHTS + 1 since the shadowed light is considered separately.
#define MAX_NUM_DIRECTIONAL_LIGHTS 8

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/ShadowPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/GPUTimers.h"
#include "Graphics/Shaders.h"
#include "Graphics/Buffers.h"
#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  struct DirectionalLightPassDataBlock
  {
    // Shadowed directional light.
    Shader* directionalEvaluationS;
    Shader* directionalEvaluationSA; // Atmospheric
    // Unshadowed directional light.
    Shader* directionalEvaluation;
    Shader* directionalEvaluationA; // Atmospheric

    // Pass parameters.
    UniformBuffer lightBlock;
    UniformBuffer cascadedShadowBlock;
    ShaderStorageBuffer atmosphereIndices;

    // Directional lights.
    uint directionalLightCount;
    std::array<DirectionalLight, MAX_NUM_DIRECTIONAL_LIGHTS> directionalLightQueue;
    uint directionalLightCountA;
    std::array<DirectionalLight, MAX_NUM_DIRECTIONAL_LIGHTS> directionalLightQueueA;
    std::array<RendererDataHandle, MAX_NUM_ATMOSPHERES> attachedAtmo;

    // The primary light and if it cast shadows.
    bool castShadows;
    RendererDataHandle primaryLightAttachedAtmo;
    DirectionalLight primaryLight;

    // Some statistics.
    float frameTime;

    DirectionalLightPassDataBlock()
      : directionalEvaluationS(nullptr)
      , directionalEvaluationSA(nullptr)
      , directionalEvaluation(nullptr)
      , directionalEvaluationA(nullptr)
      , lightBlock(8 * sizeof(DirectionalLight) + sizeof(glm::ivec4), BufferType::Dynamic)
      , cascadedShadowBlock(4 * (sizeof(glm::mat4) + sizeof(glm::vec4)) + sizeof(glm::vec4),
                            BufferType::Dynamic)
      , atmosphereIndices(MAX_NUM_DIRECTIONAL_LIGHTS * sizeof(int), BufferType::Dynamic)
      , directionalLightCount(0u)
      , directionalLightCountA(0u)
      , attachedAtmo()
      , castShadows(false)
      , primaryLightAttachedAtmo(-1)
      , primaryLight()
      , frameTime(0.0f)
    { }
  };

  class DirectionalLightPass : public RenderPass
  {
  public:
	DirectionalLightPass(Renderer3D::GlobalRendererData* globalRendererData, 
                         GeometryPass* previousGeoPass, ShadowPass* previousShadowPass, 
                         SkyAtmospherePass* previousSkyAtmoPass);
	~DirectionalLightPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;

    void submit(const DirectionalLight &light, const glm::mat4& model, 
                RendererDataHandle attachedSkyAtmosphere = -1);
    void submitPrimary(const DirectionalLight &light, bool castShadows, 
                       const glm::mat4& model, RendererDataHandle attachedSkyAtmosphere = -1);
  private:
    DirectionalLightPassDataBlock passData;

    GeometryPass* previousGeoPass;
    ShadowPass* previousShadowPass;
    SkyAtmospherePass* previousSkyAtmoPass;

    AsynchTimer timer;
  };
}