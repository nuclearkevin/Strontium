#pragma once

namespace Strontium
{
  // Data common to the renderer.
  struct GlobalRendererData;

  // Abstract render pass class.
  class RenderPass
  {
  public:
    RenderPass() = default;
    virtual ~RenderPass() = default;

    virtual void init(GlobalRendererData* dataBlock) = 0;
    virtual void update() = 0;
    virtual void shutdown() = 0;
  protected:
    virtual void* getInternalDataBlock() = 0;
  private:
  };
}
