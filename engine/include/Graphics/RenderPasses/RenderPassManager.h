#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/RenderPasses/RenderPass.h"
#include "Graphics/FrameBuffer.h"

namespace Strontium
{
  class RenderPassManager
  {
  public:
	RenderPassManager() = default;
	~RenderPassManager();

	template <typename T, typename ... Args>
	T* insertRenderPass(Args ... args)
	{
	  static_assert(std::is_base_of<RenderPass, T>::value, "Class must derive from RenderPass.");

	  RenderPass* retPass = nullptr;
	  for (auto& pass : this->renderPasses)
	  {
		retPass = dynamic_cast<T*>(pass.get());
		assert((!retPass, " Cannot have multiples of the same type of renderpass!"));
	  }

	  this->renderPasses.emplace_back(new T(std::forward<Args>(args)...));
	  this->renderPasses.back()->manager = this;

	  return static_cast<T*>(this->renderPasses.back().get());
	}

	template <typename T>
	T* getRenderPass()
	{
	  static_assert(std::is_base_of<RenderPass, T>::value, "Class must derive from RenderPass.");

	  T* retPass = nullptr;
	  for (auto& pass : this->renderPasses)
	  {
		retPass = dynamic_cast<T*>(pass.get());
		if (retPass)
		  break;
	  }

	  return retPass;
	}

	void onInit();
	void onRendererBegin(uint width, uint height);
	void onRender();
	void onRendererEnd(FrameBuffer& frontBuffer);
	void onShutdown();

  private:
	std::vector<Unique<RenderPass>> renderPasses;
  };
}