#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Buffers.h"
#include "Graphics/GPUTimers.h"

namespace Strontium
{
  struct PostProcessingPassDataBlock
  {
    Shader* postProcessingShader;

    UniformBuffer postProcessingParams;

    // Various post processing settings.
    uint toneMapOp;
    bool useFXAA;

    // Some statistics.
    float frameTime;

    PostProcessingPassDataBlock()
      : postProcessingShader(nullptr)
      , postProcessingParams(6 * sizeof(float), BufferType::Dynamic)
      , toneMapOp(0u)
      , useFXAA(true)
      , frameTime(0.0f)
    { }
  };

  class PostProcessingPass : public RenderPass
  {
  public:
	PostProcessingPass(Renderer3D::GlobalRendererData* globalRendererData,
                       GeometryPass* previousGeoPass);
	~PostProcessingPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;
  private:
    PostProcessingPassDataBlock passData;

    GeometryPass* previousGeoPass;

    AsynchTimer timer;
  };
}