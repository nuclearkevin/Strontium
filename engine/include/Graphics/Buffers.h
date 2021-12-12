// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

namespace Strontium
{
  // Draw types.
  enum class BufferType
  {
    Static = 0x88E4, // GL_STATIC_DRAW
    Dynamic = 0x88E8 // GL_DYNAMIC_DRAW
  };

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

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    // Bind/unbind the buffer.
    auto bind()   -> void;
    auto unbind() -> void;

    uint getID() { return this->bufferID; }
  protected:
    // OpenGL buffer ID.
    uint      bufferID;

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
    IndexBuffer(const uint* bufferData, unsigned numIndices, BufferType type);
    ~IndexBuffer();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    // Bind/unbind the buffer.
    auto bind()     -> void;
    auto unbind()   -> void;

    // Getters.
    auto getCount() -> unsigned;

    uint getID() { return this->bufferID; }
  protected:
    // OpenGL buffer ID.
    uint        bufferID;

    // If the buffer has data or not.
    bool          hasData;

    // Type of the buffer to prevent mismatching.
    BufferType    type;

    // The data currently in the buffer.
    const uint* bufferData;
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

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    // Bind/unbind the buffer.
    void bind();
    void bindToPoint(const uint bindPoint);
    void unbind();

    // Set a specific part of the buffer data.
    void setData(uint start, uint newDataSize, const void* newData);

    uint getID() { return this->bufferID; }
    bool hasData() { return this->filled; }
  protected:
    // OpenGL buffer ID.
    uint      bufferID;

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
    Depth24 = 0x81A6, // GL_DEPTH_COMPONENT24
    Depth32f = 0x8CAC, // GL_DEPTH_COMPONENT32F
    Stencil = 0x8D48, // GL_STENCIL_INDEX8
    DepthStencil = 0x88F0 // GL_DEPTH24_STENCIL8
  };

  class RenderBuffer
  {
  public:
    // Default constructor generates a generic render buffer with just a depth
    // attachment.
    RenderBuffer();
    RenderBuffer(uint width, uint height);
    RenderBuffer(uint width, uint height, const RBOInternalFormat &format);
    ~RenderBuffer();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    RenderBuffer(const RenderBuffer&) = delete;
    RenderBuffer& operator=(const RenderBuffer&) = delete;

    // Reset the renderbuffer.
    void reset(uint newWidth, uint newHeight);
    void reset(uint newWidth, uint newHeight, const RBOInternalFormat& newFormat);

    // Bind/unbind the buffer.
    void bind();
    void unbind();

    uint getID() { return this->bufferID; }
    RBOInternalFormat getFormat() { return this->format; }
    glm::vec2 getSize() { return glm::vec2(this->width, this->height); }
  protected:
    uint bufferID;

    RBOInternalFormat format;

    uint width, height;
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

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;

    // Bind/unbind the buffer.
    void bind();
    void bindToPoint(const uint bindPoint);
    void unbind();

    // Set the data in a region of the buffer.
    void setData(uint start, uint newDataSize, const void* newData);

    uint getID() { return this->bufferID; }
    bool hasData() { return this->filled; }
    uint size() const { return this->dataSize; }
  protected:
    // OpenGL buffer ID.
    uint bufferID;

    // If the buffer has data or not.
    bool filled;

    // Type of the buffer to prevent mismatching.
    BufferType type;

    // The size of the data currently in the buffer.
    uint dataSize;
  };
}
