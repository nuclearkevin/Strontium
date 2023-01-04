#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/LightCullingPass.h"
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

  struct CulledLightingPassDataBlock
  {
    Shader* evaluatePointLight;
    Shader* evaluateSpotLight;

    // Some statistics.
    float frameTime;

    CulledLightingPassDataBlock()
      : evaluatePointLight(nullptr)
      , frameTime(0.0f)
    { }
  };

  class CulledLightingPass final : public RenderPass
  {
  public:
    CulledLightingPass(Renderer3D::GlobalRendererData* globalRendererData, 
                       GeometryPass* previousGeoPass, LightCullingPass* previousCullPass);
	~CulledLightingPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle &handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer &frontBuffer) override;
    void onShutdown() override;
  private:
    CulledLightingPassDataBlock passData;

    GeometryPass* previousGeoPass;
    LightCullingPass* previousCullPass;

    AsynchTimer timer;
  };
}