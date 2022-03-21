#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

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
    CubeMapSeamless = 0x884F, // GL_TEXTURE_CUBE_MAP_SEAMLESS
    SmoothLines = 0x0B20 // GL_LINE_SMOOTH
  };

  enum class BlendEquation
  {
    Additive = 0x8006 // GL_FUNC_ADD
  };

  enum class BlendFunction
  {
    One = 1, // GL_ONE
    SourceAlpha = 0x0302, // GL_SRC_ALPHA
    OneMinusSourceAlpha = 0x0303 // GL_ONE_MINUS_SRC_ALPHA
  };

  enum class PrimativeType
  {
    Line = 0x0001, // GL_LINES
    Triangle = 0x0004 // GL_TRIANGLES
  };

  enum class FaceType
  {
    Front = 0x0404, // GL_FRONT
    Back = 0x0405 // GL_BACK
  };

  enum class PolygonMode
  {
    Point = 0x1B00, // GL_POINT
    Line = 0x1B01, // GL_LINE
    Fill = 0x1B02 // GL_FILL
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

    void drawElements(PrimativeType primative, uint count, const void* indices = nullptr);
    void drawArrays(PrimativeType primative, uint start, uint count);
    void drawElementsInstanced(PrimativeType primative, uint count, uint numInstances = 1, 
                               const void* indices = nullptr);
    void cullType(FaceType face);
    void setPolygonMode(const PolygonMode& mode);
  };
}
