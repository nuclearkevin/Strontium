#pragma once

// Technically MAX_NUM_DIRECTIONAL_LIGHTS + 1 since the shadowed light is considered separately.
#define MAX_NUM_DIRECTIONAL_LIGHTS 8

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/ShadowPass.h"
#include "Graphics/GPUTimers.h"
#include "Graphics/Shaders.h"
#include "Graphics/Buffers.h"
#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  struct DirectionalLightPassDataBlock
  {
    Shader* directionalEvaluationS;
    Shader* directionalEvaluation;

    // Pass parameters.
    UniformBuffer lightBlock;
    UniformBuffer cascadedShadowBlock;

    // Directional lights.
    uint directionalLightCount;
    std::array<DirectionalLight, MAX_NUM_DIRECTIONAL_LIGHTS> directionalLightQueue;

    // The primary light and if it cast shadows.
    bool castShadows;
    DirectionalLight primaryLight;

    // Some statistics.
    float frameTime;

    DirectionalLightPassDataBlock()
      : directionalEvaluationS(nullptr)
      , directionalEvaluation(nullptr)
      , lightBlock(8 * sizeof(DirectionalLight) + sizeof(glm::ivec4), BufferType::Dynamic)
      , cascadedShadowBlock(4 * (sizeof(glm::mat4) + sizeof(glm::vec4)) + sizeof(glm::vec4),
                            BufferType::Dynamic)
      , directionalLightCount(0u)
      , castShadows(false)
      , primaryLight()
      , frameTime(0.0f)
    { }
  };

  class DirectionalLightPass : public RenderPass
  {
  public:
	DirectionalLightPass(Renderer3D::GlobalRendererData* globalRendererData, 
                         GeometryPass* previousGeoPass, ShadowPass* previousShadowPass);
	~DirectionalLightPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;

    void submit(const DirectionalLight &light, const glm::mat4& model);
    void submitPrimary(const DirectionalLight &light, bool castShadows, 
                       const glm::mat4& model);
  private:
    DirectionalLightPassDataBlock passData;

    GeometryPass* previousGeoPass;
    ShadowPass* previousShadowPass;

    AsynchTimer timer;
  };
}