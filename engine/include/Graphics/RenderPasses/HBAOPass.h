#pragma once

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/RenderPasses/RenderPass.h"
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

  class GeometryPass;
  class HiZPass;

  struct HBAOPassDataBlock
  {
	Shader* aoCompute;
	Shader* aoBlur;

	Texture2D downsampleAO;

	UniformBuffer aoParamsBuffer;

	bool enableAO;
	float aoRadius;
	float aoMultiplier;
	float aoExponent;

	// Some statistics to display.
    float frameTime;

	HBAOPassDataBlock()
	  : aoCompute(nullptr)
	  , aoBlur(nullptr)
	  , aoParamsBuffer(2 * sizeof(glm::vec4), BufferType::Dynamic)
	  , enableAO(false)
	  , aoRadius(0.001f)
	  , aoMultiplier(1.0f)
	  , aoExponent(1.0f)
	  , frameTime(0.0f)
	{ }
  };

  class HBAOPass final : public RenderPass
  {
  public:
	HBAOPass(Renderer3D::GlobalRendererData* globalRendererData,
		     GeometryPass* previousGeoPass, HiZPass* previousHiZPass);
	~HBAOPass();

	void onInit() override;
	void updatePassData() override;
	RendererDataHandle requestRendererData() override;
	void deleteRendererData(RendererDataHandle& handle) override;
	void onRendererBegin(uint width, uint height) override;
	void onRender() override;
	void onRendererEnd(FrameBuffer& frontBuffer) override;
	void onShutdown() override;
  private:
    HBAOPassDataBlock passData;

	GeometryPass* previousGeoPass;
	HiZPass* previousHiZPass;

	AsynchTimer timer;
  };
}