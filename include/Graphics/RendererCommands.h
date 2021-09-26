#pragma once

// Macro include file.
#include "StrontiumPCH.h"

namespace Strontium
{
  // Depth functions. Addition to this as they are required.
  enum class DepthFunctions
  {
    Less = 0x0201, // GL_LESS
    LEq = 0x0203 // GL_LEQUAL
  };

  // Render functions to glEnable. Adding to this as they are required.
  enum class RendererFunction
  {
    DepthTest = 0x0B71, // GL_DEPTH_TEST
    Blending = 0x0BE2, // GL_BLEND
    CubeMapSeamless = 0x884F // GL_TEXTURE_CUBE_MAP_SEAMLESS
  };

  enum class BlendEquation
  {
    Additive = 0x8006 // GL_FUNC_ADD
  };

  enum class BlendFunction
  {
    One = 1 // GL_ONE
  };

  enum class PrimativeType
  {
    Triangle = 0x0004 // GL_TRIANGLES
  };

  namespace RendererCommands
  {
    void enable(const RendererFunction &toEnable);
    void disable(const RendererFunction &toDisable);
    void enableDepthMask();
    void disableDepthMask();
    void blendEquation(const BlendEquation &equation);
    void blendFunction(const BlendFunction &source, const BlendFunction &target);
    void depthFunction(const DepthFunctions &function);
    void setClearColour(const glm::vec4 &colour);
    void clear(const bool &clearColour = true, const bool &clearDepth = true,
               const bool &clearStencil = true);
    void setViewport(const glm::ivec2 topRight, const glm::ivec2 bottomLeft = glm::ivec2(0));

    void drawPrimatives(PrimativeType primative, uint count, const void* indices = nullptr);
  };
}
