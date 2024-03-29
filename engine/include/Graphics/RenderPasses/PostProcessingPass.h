#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/BloomPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Buffers.h"
#include "Graphics/GPUTimers.h"

namespace Strontium
{
  namespace Renderer3D
  {
    struct GlobalRendererData;
  }

  struct PostProcessingPassDataBlock
  {
    Shader* postProcessingShader;

    UniformBuffer postProcessingParams;

    // Various post processing settings.
    uint toneMapOp;
    bool useFXAA;
    bool useGrid;
    bool drawOutline;

    // Some statistics.
    float frameTime;

    PostProcessingPassDataBlock()
      : postProcessingShader(nullptr)
      , postProcessingParams(6 * sizeof(float), BufferType::Dynamic)
      , toneMapOp(5u)
      , useFXAA(true)
      , useGrid(true)
      , drawOutline(false)
      , frameTime(0.0f)
    { }
  };

  class PostProcessingPass final : public RenderPass
  {
  public:
	PostProcessingPass(Renderer3D::GlobalRendererData* globalRendererData,
                       GeometryPass* previousGeoPass,
                       BloomPass* previousBloomPass);
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
    BloomPass* previousBloomPass;

    AsynchTimer timer;
  };
}