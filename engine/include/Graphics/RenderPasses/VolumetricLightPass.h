#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/RenderPasses/GeometryPass.h"
#include "Graphics/RenderPasses/HiZPass.h"
#include "Graphics/RenderPasses/GodrayPass.h"

#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"
#include "Graphics/GPUTimers.h"

namespace Strontium
{
  namespace Renderer3D
  {
    struct GlobalRendererData;
  }

  struct VolumetricLightPassDataBlock
  {
    Shader* volumetricApplication;

    UniformBuffer volumetricParamBuffer;

    // Some statistics to display.
    float frameTime;

    VolumetricLightPassDataBlock()
      : volumetricApplication(nullptr)
      , volumetricParamBuffer(sizeof(glm::ivec4), BufferType::Dynamic)
      , frameTime(0.0f)
    { }
  };

  class VolumetricLightPass final : public RenderPass
  {
  public:
    VolumetricLightPass(Renderer3D::GlobalRendererData* globalRendererData, 
                        GeometryPass* previousGeoPass, HiZPass* previousHiZPass, 
                        GodrayPass* previousGodrayPass);
    ~VolumetricLightPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle & handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer & frontBuffer) override;
    void onShutdown() override;

  private:
    VolumetricLightPassDataBlock passData;

    GeometryPass* previousGeoPass;
    HiZPass* previousHiZPass;
    GodrayPass* previousGodrayPass;

    AsynchTimer timer;
  };
}