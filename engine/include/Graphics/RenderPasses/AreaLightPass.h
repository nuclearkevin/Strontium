#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/GPUTimers.h"
#include "Graphics/Shaders.h"
#include "Graphics/Buffers.h"
#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  namespace Renderer3D
  {
    struct GlobalRendererData;
  }

  struct AreaLightPassDataBlock
  {
	// Some statistics.
    float frameTime;

	AreaLightPassDataBlock()
	  : frameTime(0.0f)
	{ }
  };

  class AreaLightPass final : public RenderPass
  {
  public:
	AreaLightPass(Renderer3D::GlobalRendererData* globalRendererData, 
                  GeometryPass* previousGeoPass);
	~AreaLightPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;

  private:
    AreaLightPassDataBlock passData;

    GeometryPass* previousGeoPass;

    AsynchTimer timer;
  };
}