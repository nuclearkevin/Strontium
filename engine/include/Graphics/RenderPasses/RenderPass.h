#pragma once

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/FrameBuffer.h"

namespace Strontium
{
  // Data common to the renderer.
  namespace Renderer3D
  {
    struct GlobalRendererData;
  }

  using RendererDataHandle = int;

  class RenderPassManager;

  // Abstract renderpass class.
  class RenderPass
  {
  public:
    RenderPass(void* internalDataBlock, Renderer3D::GlobalRendererData* globalRendererData, 
               const std::vector<RenderPass*> &previousPasses)
      : globalBlock(globalRendererData)
      , internalDataBlock(internalDataBlock)
      , previousPasses(previousPasses)
      , manager(nullptr)
    { }

    virtual ~RenderPass() = default;

    virtual void onInit() = 0;
    virtual void updatePassData() = 0;
    virtual RendererDataHandle requestRendererData() = 0;
    virtual void deleteRendererData(RendererDataHandle& handle) = 0;
    virtual void onRendererBegin(uint width, uint height) = 0;
    virtual void onRender() = 0;
    virtual void onRendererEnd(FrameBuffer& frontBuffer) = 0;
    virtual void onShutdown() = 0;

    template <typename T>
    T* getInternalDataBlock()
    {
      return static_cast<T*>(this->internalDataBlock);
    };

  protected:
    Renderer3D::GlobalRendererData* globalBlock;
    void* internalDataBlock;

    std::vector<RenderPass*> previousPasses;
    RenderPassManager* manager;

    friend class RenderPassManager;
  };
}
