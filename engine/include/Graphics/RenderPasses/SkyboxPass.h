#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/SkyAtmospherePass.h"
#include "Graphics/GPUTimers.h"
#include "Graphics/Shaders.h"
#include "Graphics/Buffers.h"

namespace Strontium
{
  struct SkyboxPassDataBlock
  {
    Shader* dynamicSkyEvaluation;

    UniformBuffer skyboxParams;

    // Packed skybox parameters. TODO: Handle more then one skybox.
    std::vector<glm::vec4> skyboxes;

    // Some statistics.
    float frameTime;

    SkyboxPassDataBlock()
      : dynamicSkyEvaluation(nullptr)
      , skyboxParams(sizeof(glm::vec4), BufferType::Dynamic)
      , frameTime(0.0f)
    { }
  };

  class SkyboxPass : public RenderPass
  {
  public:
	SkyboxPass(Renderer3D::GlobalRendererData* globalRendererData, 
               GeometryPass* previousGeoPass,
			   SkyAtmospherePass* previousSkyAtmoPass);
	~SkyboxPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;

    // Submit a dynamic sky.
    void submit(RendererDataHandle skyAtmosphereIndex, float sunSize, float skyIntensity);
  private:
    SkyboxPassDataBlock passData;

    GeometryPass* previousGeoPass;
    SkyAtmospherePass* previousSkyAtmoPass;

    AsynchTimer timer;
  };
}