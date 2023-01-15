#pragma once

#define MAX_NUM_DYNAMIC_IBL 8

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
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

  struct DynamicSkyIBLPassDataBlock
  {
    Shader* shDynamicSkyIrradiance;
    Shader* shReduce;
    Shader* shCompact;
    Shader* dynamicSkyRadiance;

    CubeMapArrayTexture radianceCubemaps;

    UniformBuffer iblParamsBuffer;
    ShaderStorageBuffer iblIndices;

    UniformBuffer diffuseSHParams;
    ShaderStorageBuffer diffuseSHStorage;
    ShaderStorageBuffer compactedDiffuseSH;

    // The IBL map parameters.
    std::bitset<MAX_NUM_DYNAMIC_IBL> updateIBL;
    std::array<DynamicIBL, MAX_NUM_DYNAMIC_IBL> iblQueue;

    // Global IBL parameters.
    uint numRadSamples;

    // Some statistics.
    float frameTime;

    DynamicSkyIBLPassDataBlock()
      : shDynamicSkyIrradiance(nullptr)
      , shReduce(nullptr)
      , shCompact(nullptr)
      , dynamicSkyRadiance(nullptr)
      , iblParamsBuffer(sizeof(glm::ivec4), BufferType::Dynamic)
      , iblIndices((MAX_NUM_ATMOSPHERES + MAX_NUM_DYNAMIC_IBL) * sizeof(int), BufferType::Dynamic)
      , diffuseSHParams(sizeof(glm::ivec4), BufferType::Dynamic)
      , diffuseSHStorage(10 * sizeof(glm::vec4) * 512 * MAX_NUM_DYNAMIC_IBL, BufferType::Dynamic)
      , compactedDiffuseSH(10 * sizeof(glm::vec4) * MAX_NUM_DYNAMIC_IBL, BufferType::Dynamic)
      , numRadSamples(64)
      , frameTime(0.0f)
    { }
  };

  class DynamicSkyIBLPass final : public RenderPass
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

    void submit(const DynamicIBL &iblParams, bool skyUpdated);

  private:
    DynamicSkyIBLPassDataBlock passData;

    // Stacks for the inactive and active IBL components.
    std::stack<RendererDataHandle> availableHandles;
    std::vector<RendererDataHandle> activeHandles;
    std::vector<RendererDataHandle> updatableHandles;

    std::vector<RendererDataHandle> updatedSkyHandles;

    SkyAtmospherePass* previousSkyAtmoPass;

    AsynchTimer timer;
  };
}