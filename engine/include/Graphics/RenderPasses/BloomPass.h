#pragma once

#define MAX_NUM_BLOOM_MIPS 7

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"
#include "Graphics/GPUTimers.h"

namespace Strontium
{
  struct BloomPassDataBlock
  {
    Shader* bloomDownsampleKaris;
    Shader* bloomDownsample;
    Shader* bloomCopy;
    Shader* bloomUpsampleBlend;

    UniformBuffer bloomParams;

    Texture2D downsampleBuffer;
    Texture2D upsampleBuffer1;
    Texture2D upsampleBuffer2;

    // Bloom settings.
    bool useBloom;
    float threshold;
    float knee;
    float radius;
    float intensity;

    // Some statistics.
    float frameTime;

    BloomPassDataBlock()
      : bloomDownsampleKaris(nullptr)
      , bloomDownsample(nullptr)
      , bloomCopy(nullptr)
      , bloomUpsampleBlend(nullptr)
      , bloomParams(2 * sizeof(glm::vec4), BufferType::Dynamic)
      , useBloom(false)
      , threshold(1.0f)
      , knee(1.0f)
      , radius(1.0f)
      , intensity(1.0f)
      , frameTime(0.0f)
    { }
  };

  class BloomPass : public RenderPass
  {
  public:
	BloomPass(Renderer3D::GlobalRendererData* globalRendererData);
	~BloomPass() override;

    void onInit() override;
    void updatePassData() override;
    RendererDataHandle requestRendererData() override;
    void deleteRendererData(RendererDataHandle& handle) override;
    void onRendererBegin(uint width, uint height) override;
    void onRender() override;
    void onRendererEnd(FrameBuffer& frontBuffer) override;
    void onShutdown() override;
  private:
    BloomPassDataBlock passData;

    AsynchTimer timer;
  };
}