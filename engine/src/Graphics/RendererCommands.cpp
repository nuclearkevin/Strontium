#include "Graphics/RendererCommands.h"

// OpenGL includes.
#include "glad/glad.h"

namespace Strontium
{
  void
  RendererCommands::enable(const RendererFunction &toEnable)
  {
    glEnable(static_cast<GLenum>(toEnable));
  }

  void
  RendererCommands::disable(const RendererFunction &toDisable)
  {
    glDisable(static_cast<GLenum>(toDisable));
  }

  void
  RendererCommands::enableDepthMask()
  {
    glDepthMask(GL_TRUE);
  }

  void
  RendererCommands::disableDepthMask()
  {
    glDepthMask(GL_FALSE);
  }

  void
  RendererCommands::blendEquation(const BlendEquation &equation)
  {
    glBlendEquation(static_cast<GLenum>(equation));
  }

  void
  RendererCommands::blendFunction(const BlendFunction &source,
                                  const BlendFunction &target)
  {
    glBlendFunc(static_cast<GLenum>(source), static_cast<GLenum>(target));
  }

  void
  RendererCommands::depthFunction(const DepthFunctions &function)
  {
    glDepthFunc(static_cast<GLenum>(function));
  }

  void
  RendererCommands::setClearColour(const glm::vec4 &colour)
  {
    glClearColor(colour.r, colour.b, colour.g, colour.a);
  }

  void
  RendererCommands::clear(const bool &clearColour, const bool &clearDepth,
                          const bool &clearStencil)
  {
    if (clearColour)
      glClear(GL_COLOR_BUFFER_BIT);
    if (clearDepth)
      glClear(GL_DEPTH_BUFFER_BIT);
    if (clearStencil)
      glClear(GL_STENCIL_BUFFER_BIT);
  }

  void
  RendererCommands::setViewport(const glm::ivec2 topRight,
                                const glm::ivec2 bottomLeft)
  {
    glViewport(bottomLeft.x, bottomLeft.y, topRight.x, topRight.y);
  }

  void
  RendererCommands::drawElements(PrimativeType primative, uint count,
                                   const void* indices)
  {
    glDrawElements(static_cast<GLenum>(primative), count, GL_UNSIGNED_INT, indices);
  }

  void 
  RendererCommands::drawElementsInstanced(PrimativeType primative, uint count, uint numInstances,
                                          const void* indices)
  {
    glDrawElementsInstanced(static_cast<GLenum>(primative), count, GL_UNSIGNED_INT, indices, numInstances);
  }

  void 
  RendererCommands::drawArrays(PrimativeType primative, uint start, uint count)
  {
    glDrawArrays(static_cast<GLenum>(primative), start, count);
  }

  void 
  RendererCommands::drawArraysInstanced(PrimativeType primative, uint start, uint count, uint numInstances)
  {
    glDrawArraysInstanced(static_cast<GLenum>(primative), start, count, numInstances);
  }

  void 
  RendererCommands::cullType(FaceType face)
  {
    glCullFace(static_cast<GLenum>(face));
  }

  void 
  RendererCommands::setPolygonMode(const PolygonMode& mode)
  {
    glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(mode));
  }
}
