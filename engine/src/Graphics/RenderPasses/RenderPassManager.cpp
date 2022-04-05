#include "Graphics/RenderPasses/RenderPassManager.h"

namespace Strontium
{
  void 
  RenderPassManager::onInit()
  {
	for (auto& pass : this->renderPasses)
	  pass->onInit();
  }

  void 
  RenderPassManager::onRendererBegin(uint width, uint height)
  {
	for (auto& pass : this->renderPasses)
	  pass->onRendererBegin(width, height);
  }

  void 
  RenderPassManager::onRender()
  {
	for (auto& pass : this->renderPasses)
	  pass->onRender();
  }

  void 
  RenderPassManager::onRendererEnd(FrameBuffer& frontBuffer)
  {
	for (auto& pass : this->renderPasses)
	  pass->onRendererEnd(frontBuffer);
  }

  void 
  RenderPassManager::onShutdown()
  {
	for (auto& pass : this->renderPasses)
	  pass->onShutdown();
  }
}