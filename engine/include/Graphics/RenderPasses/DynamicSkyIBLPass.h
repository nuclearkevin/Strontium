#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"

namespace Strontium
{
  struct DynamicSkyIBLPassDataBlock
  {
    Shader* dynamicSkyIrradiance;
    Shader* dynamicSkyRadiance;

    CubeMapArrayTexture irradianceCubemaps;
    CubeMapArrayTexture radianceCubemaps;

    DynamicSkyIBLPassDataBlock()
      : dynamicSkyIrradiance(nullptr)
      , dynamicSkyRadiance(nullptr)
    { }
  };

  class DynamicSkyIBLPass : public RenderPass
  {
  public:
	DynamicSkyIBLPass(Renderer3D::GlobalRendererData* globalRendererData);
	~DynamicSkyIBLPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(const RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;

  private:
    DynamicSkyIBLPassDataBlock passData;
  };
}