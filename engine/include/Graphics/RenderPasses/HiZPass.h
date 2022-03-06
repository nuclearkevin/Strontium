#pragma once

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/GPUTimers.h"

namespace Strontium
{
  class GeometryPass;

  struct HiZPassDataBlock
  {
    Shader* depthCopy;
    Shader* depthReduction;

    Texture2D hierarchicalDepth;

    // Some statistics to display.
    
    float frameTime;

    HiZPassDataBlock()
      : depthCopy(nullptr)
      , depthReduction(nullptr)
      , frameTime(0.0f)
    { }
  };

  class HiZPass : public RenderPass
  {
  public:
    HiZPass(Renderer3D::GlobalRendererData* globalRendererData, 
            GeometryPass* previousGeoPass);
    ~HiZPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;
  private:
    HiZPassDataBlock passData;

    GeometryPass* previousGeoPass;

    AsynchTimer timer;
  };
}