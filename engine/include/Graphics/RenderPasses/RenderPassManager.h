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
	~RenderPassManager() = default;

	template <typename T, typename ... Args>
	T* insertRenderPass(Args ... args)
	{
	  static_assert(std::is_base_of<RenderPass, T>::value, "Class must derive from RenderPass.");

	  auto typeHash = typeid(T).hash_code();
	  assert((this->hashToLinear.find(typeHash) != this->hashToLinear.end(), " Cannot have multiples of the same renderpass."));

	  this->renderPasses.emplace_back(new T(std::forward<Args>(args)...));
	  this->renderPasses.back()->manager = this;

	  this->hashToLinear.emplace(typeHash, this->renderPasses.size() - 1);

	  return static_cast<T*>(this->renderPasses.back().get());
	}

	template <typename T>
	T* getRenderPass()
	{
	  static_assert(std::is_base_of<RenderPass, T>::value, "Class must derive from RenderPass.");

	  auto typeHash = typeid(T).hash_code();
	  return static_cast<T*>(this->renderPasses[this->hashToLinear.at(typeHash)].get());
	}

	void onInit();
	void onRendererBegin(uint width, uint height);
	void onRender();
	void onRendererEnd(FrameBuffer& frontBuffer);
	void onShutdown();

  private:
	std::vector<Unique<RenderPass>> renderPasses;
	robin_hood::unordered_flat_map<std::size_t, std::size_t> hashToLinear;
  };
}