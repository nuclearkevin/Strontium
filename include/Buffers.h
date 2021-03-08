// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

namespace SciRenderer
{
  // Draw types.
  enum BufferType {STATIC = GL_STATIC_DRAW, DYNAMIC = GL_DYNAMIC_DRAW};

  // Vertex buffer class.
  class VertexBuffer
  {
  public:
    // Constructor and destructor.
    VertexBuffer(const void* bufferData, const unsigned &dataSize,
                 BufferType bufferType);
    ~VertexBuffer();

    // Bind/unbind the buffer.
    void bind();
    void unbind();

    // Set the buffer data. TODO
    void setData();

  protected:
    // OpenGL buffer ID.
    GLuint      bufferID;

    // If the buffer has data or not.
    bool        hasData;

    // Type of the buffer to prevent mismatching.
    BufferType  type;

    // The data currently in the buffer.
    const void* bufferData;
    unsigned    dataSize;
  };

  // Index buffer class.
  class IndexBuffer
  {
  public:
    // Constructor and destructor.
    IndexBuffer(const GLuint* bufferData, unsigned numIndices, BufferType type);
    ~IndexBuffer();

    // Bind/unbind the buffer.
    void bind();
    void unbind();

    // Set the buffer data. TODO
    void setData();

    // Getters.
    unsigned getCount();

  protected:
    // OpenGL buffer ID.
    GLuint        bufferID;

    // If the buffer has data or not.
    bool          hasData;

    // Type of the buffer to prevent mismatching.
    BufferType    type;

    // The data currently in the buffer.
    const GLuint* bufferData;
    unsigned      count;
  };

  // Frame buffer class.
  class Framebuffer
  {
  public:
    // A constructor to generate a framebuffer at a particular location or any
    // location.
    Framebuffer();
    Framebuffer(GLuint bufferLocation);
    ~Framebuffer();

    // Bind and unbind the framebuffer. Have to unbind before rendering to the
    // default buffer.
    void bind();
    void unbind();

    // Methods to add/remove textures and a depth buffer.
    void attachTexture2D();
    void attachDepthBuffer();

    void unattachTexture2D();
    void unattachDepthBuffer();
  protected:
    GLuint framebufferID;

  };
}
