// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

namespace Strontium
{
  // Draw types.
  enum class BufferType { Static = GL_STATIC_DRAW, Dynamic = GL_DYNAMIC_DRAW };

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

    GLuint getID() { return this->bufferID; }
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

    GLuint getID() { return this->bufferID; }
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
    UniformBuffer(const void* bufferData, const unsigned &dataSize,
                  BufferType bufferType);
    UniformBuffer(const unsigned &bufferSize, BufferType bufferType);
    UniformBuffer();
    ~UniformBuffer();

    // Bind/unbind the buffer.
    void bind();
    void bindToPoint(const GLuint bindPoint);
    void unbind();

    // Set a specific part of the buffer data.
    void setData(GLuint start, GLuint newDataSize, const void* newData);

    GLuint getID() { return this->bufferID; }
    bool hasData() { return this->filled; }
  protected:
    // OpenGL buffer ID.
    GLuint      bufferID;

    // If the buffer has data or not.
    bool        filled;

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
    Depth24 = GL_DEPTH_COMPONENT24,
    Depth32f = GL_DEPTH_COMPONENT32F,
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

    // Bind/unbind the buffer.
    void bind();
    void unbind();

    GLuint getID() { return this->bufferID; }
    RBOInternalFormat getFormat() { return this->format; }
    glm::vec2 getSize() { return glm::vec2(this->width, this->height); }
  protected:
    GLuint bufferID;

    RBOInternalFormat format;

    GLuint width, height;
  };

  //----------------------------------------------------------------------------
  // Shader storage buffer here.
  //----------------------------------------------------------------------------
  class ShaderStorageBuffer
  {
  public:
    ShaderStorageBuffer(const void* bufferData, const unsigned &dataSize,
                        BufferType bufferType);
    ShaderStorageBuffer(const unsigned &bufferSize, BufferType bufferType);
    ~ShaderStorageBuffer();

    // Bind/unbind the buffer.
    void bind();
    void bindToPoint(const GLuint bindPoint);
    void unbind();

    // Set the data in a region of the buffer.
    void setData(GLuint start, GLuint newDataSize, const void* newData);

    GLuint getID() { return this->bufferID; }
    bool hasData() { return this->filled; }
    GLuint size() const { return this->dataSize; }
  protected:
    // OpenGL buffer ID.
    GLuint bufferID;

    // If the buffer has data or not.
    bool filled;

    // Type of the buffer to prevent mismatching.
    BufferType type;

    // The size of the data currently in the buffer.
    GLuint dataSize;
  };
}
