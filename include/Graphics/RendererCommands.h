#pragma once

// Macro include file.
#include "StrontiumPCH.h"

namespace Strontium
{
  // Depth functions. Addition to this as they are required.
  enum class DepthFunctions
  {
    Less = GL_LESS,
    LEq = GL_LEQUAL
  };

  // Render functions to glEnable. Adding to this as they are required.
  enum class RendererFunction
  {
    DepthTest = GL_DEPTH_TEST,
    Blending = GL_BLEND,
    CubeMapSeamless = GL_TEXTURE_CUBE_MAP_SEAMLESS
  };

  enum class BlendEquation
  {
    Additive = GL_FUNC_ADD
  };

  enum class BlendFunction
  {
    One = GL_ONE
  };

  enum class PrimativeType
  {
    Triangle = GL_TRIANGLES
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

    void drawPrimatives(PrimativeType primative, GLuint count, const void* indices = nullptr);
  };
}
