// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

namespace SciRenderer
{
  // Draw types.
  enum class BufferType {Static = GL_STATIC_DRAW, Dynamic = GL_DYNAMIC_DRAW};

  //----------------------------------------------------------------------------
  // Vertex buffer here.
  //----------------------------------------------------------------------------
  class VertexBuffer
  {
  public:
    // Constructor and destructor.
    VertexBuffer(const void* bufferData, const unsigned &dataSize,
                 BufferType bufferType);
    ~VertexBuffer();

    // Bind/unbind the buffer.
    auto bind()   -> void;
    auto unbind() -> void;

    inline GLuint getID() { return this->bufferID; }
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

  //----------------------------------------------------------------------------
  // Index buffer here.
  //----------------------------------------------------------------------------
  class IndexBuffer
  {
  public:
    // Constructor and destructor.
    IndexBuffer(const GLuint* bufferData, unsigned numIndices, BufferType type);
    ~IndexBuffer();

    // Bind/unbind the buffer.
    auto bind()     -> void;
    auto unbind()   -> void;

    // Getters.
    auto getCount() -> unsigned;

    inline GLuint getID() { return this->bufferID; }
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

  //----------------------------------------------------------------------------
  // Uniform buffer here.
  //----------------------------------------------------------------------------
  class UniformBuffer
  {
  public:
    // Constructors and destructor.
    UniformBuffer(const void* bufferData, const unsigned &dataSize,
                  BufferType bufferType);
    UniformBuffer(const unsigned &bufferSize, BufferType bufferType);
    ~UniformBuffer();

    // Bind/unbind the buffer.
    void bindToPoint(GLuint point);
    void bind();
    void unbind();

    // Set a specific part of the buffer data.
    void setData(GLuint start, GLuint newDataSize, const void* newData);

    inline GLuint getID() { return this->bufferID; }
  protected:
    // OpenGL buffer ID.
    GLuint      bufferID;

    // If the buffer has data or not.
    bool        hasData;

    // Type of the buffer to prevent mismatching.
    BufferType  type;

    // The data currently in the buffer.
    unsigned    dataSize;
  };

  //----------------------------------------------------------------------------
  // Render buffer here.
  //----------------------------------------------------------------------------
  enum class RBOInternalFormat
  {
    Depth = GL_DEPTH_COMPONENT24,
    Stencil = GL_STENCIL_INDEX8,
    DepthStencil = GL_DEPTH24_STENCIL8
  };

  class RenderBuffer
  {
  public:
    // Default constructor generates a generic render buffer with just a depth
    // attachment.
    RenderBuffer(GLuint width, GLuint height);
    RenderBuffer(GLuint width, GLuint height, const RBOInternalFormat &format);
    ~RenderBuffer();

    void bind();
    void unbind();

    inline GLuint getID() { return this->bufferID; }
    inline RBOInternalFormat getFormat() { return this->format; }
    inline glm::vec2 getSize() { return glm::vec2(this->width, this->height); }
  protected:
    GLuint bufferID;

    RBOInternalFormat format;

    GLuint width, height;
  };
}
