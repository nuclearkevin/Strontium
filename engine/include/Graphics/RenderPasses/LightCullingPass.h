#pragma once

#define MAX_NUM_POINT_LIGHTS 1024
#define MAX_NUM_RECT_LIGHTS 64

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"
#include "Graphics/GPUTimers.h"
#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  namespace Renderer3D
  {
    struct GlobalRendererData;
  }

  struct LightCullingPassDataBlock
  {
    Shader* buildTileData;
    Shader* cullPointLights;
    Shader* cullRectLights;

    // Pass parameters.
    ShaderStorageBuffer cullingData;

    ShaderStorageBuffer pointLights;
    ShaderStorageBuffer pointLightListBuffer;

    UniformBuffer rectLights;
    ShaderStorageBuffer rectLightListBuffer;

    // Number of tiles.
    glm::uvec2 tiles;

    // Point lights.
    uint pointLightCount;
    std::array<PointLight, MAX_NUM_POINT_LIGHTS> pointLightQueue;

    // Rectangular area lights.
    uint rectAreaLightCount;
    std::array<RectAreaLight, MAX_NUM_RECT_LIGHTS> rectLightQueue;

    // Some statistics.
    float frameTime;

    LightCullingPassDataBlock()
      : buildTileData(nullptr)
      , cullPointLights(nullptr)
      , cullRectLights(nullptr)
      , cullingData(0u, BufferType::Dynamic)
      , pointLights(MAX_NUM_POINT_LIGHTS * sizeof(PointLight) + sizeof(uint), BufferType::Dynamic)
      , pointLightListBuffer(0u, BufferType::Dynamic)
      , rectLights(MAX_NUM_RECT_LIGHTS * sizeof(RectAreaLight) + sizeof(glm::ivec4), BufferType::Dynamic)
      , rectLightListBuffer(0u, BufferType::Dynamic)
      , tiles(static_cast<glm::uvec2>(glm::ceil(glm::vec2(1600.0f, 900.0f) / 16.0f)))
      , pointLightCount(0u)
      , rectAreaLightCount(0u)
      , frameTime(0.0f)
    { }
  };

  class LightCullingPass final : public RenderPass
  {
  public:
    LightCullingPass(Renderer3D::GlobalRendererData* globalRendererData, 
                     GeometryPass* previousGeoPass);
	~LightCullingPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle &handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer &frontBuffer) override;
    void onShutdown() override;

    void submit(const PointLight &light, const glm::mat4 &model);
    void submit(const RectAreaLight &light, const glm::mat4 &model, float radius, 
                bool twoSided, bool cull);

  private:
    LightCullingPassDataBlock passData;

    GeometryPass* previousGeoPass;

    AsynchTimer timer;
  };
}