#pragma once

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

  enum class MapBufferAccess
  {
    Read = 0x0001, // GL_MAP_READ_BIT
    Write = 0x0002 // GL_MAP_WRITE_BIT
  };

  enum class MapBufferSynch
  {
    Persistent = 0x0040, // GL_MAP_PERSISTENT_BIT
    Coherent = 0x0080, // GL_MAP_COHERENT_BIT
    PersistentCoherent = 0x0040 | 0x0080, // GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
    None = 0x0000 // No access parameter specified.
  };

  //----------------------------------------------------------------------------
  // Vertex buffer here.
  //----------------------------------------------------------------------------
  class VertexBuffer
  {
  public:
    VertexBuffer(const void* bufferData, uint dataSize,
                 BufferType bufferType);
    VertexBuffer(BufferType bufferType);
    ~VertexBuffer();

    // Delete the copy constructor and the assignment operator. Prevents
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer& operator=(VertexBuffer&&) = delete;

    // Bind/unbind the buffer.
    void bind();
    void unbind();

    // Modify the buffer's contents.
    void resize(uint bufferSize, BufferType bufferType);
    void setData(uint start, uint newDataSize, const void* newData);

    uint getID() { return this->bufferID; }
  private:
    // OpenGL buffer ID.
    uint bufferID;

    // If the buffer has data or not.
    bool hasData;

    // Type of the buffer to prevent mismatching.
    BufferType type;

    uint dataSize;
  };

  //----------------------------------------------------------------------------
  // Index buffer here.
  //----------------------------------------------------------------------------
  class IndexBuffer
  {
  public:
    // Constructor and destructor.
    IndexBuffer(const uint* bufferData, uint numIndices, BufferType type);
    IndexBuffer(BufferType type);
    ~IndexBuffer();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer& operator=(IndexBuffer&&) = delete;

    // Bind/unbind the buffer.
    void bind();
    void unbind();

    // Modify the buffer's contents.
    void resize(uint newCount, BufferType bufferType);
    void setData(uint start, uint newDataCount, const uint* newData);

    // Getters.
    uint getCount() const { return this->count; }

    uint getID() { return this->bufferID; }
  private:
    // OpenGL buffer ID.
    uint bufferID;

    // If the buffer has data or not.
    bool hasData;

    // Type of the buffer to prevent mismatching.
    BufferType type;

    uint count;
  };

  //----------------------------------------------------------------------------
  // Uniform buffer here.
  //----------------------------------------------------------------------------
  class UniformBuffer
  {
  public:
    UniformBuffer(const void* bufferData, uint dataSize,
                  BufferType bufferType);
    UniformBuffer(uint bufferSize, BufferType bufferType);
    UniformBuffer();
    ~UniformBuffer();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer& operator=(UniformBuffer&&) = delete;

    // Bind/unbind the buffer.
    void bind();
    void bindToPoint(const uint bindPoint);
    void unbind();

    // Resize the buffer.
    void resize(uint bufferSize, BufferType bufferType);

    // Set a specific part of the buffer data.
    void setData(uint start, uint newDataSize, const void* newData);

    uint getID() { return this->bufferID; }
    bool hasData() { return this->filled; }
  private:
    // OpenGL buffer ID.
    uint bufferID;

    // If the buffer has data or not.
    bool filled;

    // Type of the buffer to prevent mismatching.
    BufferType type;

    // The data currently in the buffer.
    uint dataSize;
  };

  //----------------------------------------------------------------------------
  // Shader storage buffer here.
  //----------------------------------------------------------------------------
  class ShaderStorageBuffer
  {
  public:
    ShaderStorageBuffer(const void* bufferData, uint bufferSize, BufferType bufferType);
    ShaderStorageBuffer(uint bufferSize, BufferType bufferType);
    ~ShaderStorageBuffer();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer(ShaderStorageBuffer&&) = delete;
    ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer& operator=(ShaderStorageBuffer&&) = delete;

    // Bind/unbind the buffer.
    void bind();
    void bindToPoint(const uint bindPoint);
    void unbind();

    // Resize the buffer.
    void resize(uint bufferSize, BufferType bufferType);

    // Set the data in a region of the buffer.
    void setData(uint start, uint newDataSize, const void* newData);

    // Copy data between buffers.
    void copyDataFromSource(ShaderStorageBuffer &source, uint readStart, uint writeStart, uint dataSize);
    void copyDataToTarget(ShaderStorageBuffer &target, uint readStart, uint writeStart, uint dataSize);

    // Map and unmap the buffer to client memory.
    void* mapBuffer(uint offset, uint length, MapBufferAccess access, MapBufferSynch sych);
    void unmapBuffer();

    uint getID() { return this->bufferID; }
    bool hasData() { return this->filled; }
    uint size() const { return this->dataSize; }
  private:
    // OpenGL buffer ID.
    uint bufferID;

    // If the buffer has data or not.
    bool filled;

    // Type of the buffer to prevent mismatching.
    BufferType type;

    // The size of the data currently in the buffer.
    uint dataSize;
  };

  //----------------------------------------------------------------------------
  // Draw indirect buffer here.
  //----------------------------------------------------------------------------
  class DrawIndirectBuffer
  {
  public:
    DrawIndirectBuffer(const void* bufferData, uint bufferSize, BufferType bufferType);
    DrawIndirectBuffer(uint bufferSize, BufferType bufferType);
    ~DrawIndirectBuffer();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    DrawIndirectBuffer(const DrawIndirectBuffer&) = delete;
    DrawIndirectBuffer(DrawIndirectBuffer&&) = delete;
    DrawIndirectBuffer& operator=(const DrawIndirectBuffer&) = delete;
    DrawIndirectBuffer& operator=(DrawIndirectBuffer&&) = delete;

    // Bind/unbind the buffer.
    void bind();
    void bindToPoint(const uint bindPoint);
    void unbind();

    // Resize the buffer.
    void resize(uint bufferSize, BufferType bufferType);

    // Set the data in a region of the buffer.
    void setData(uint start, uint newDataSize, const void* newData);

    // Copy data between buffers.
    void copyDataFromSource(DrawIndirectBuffer &source, uint readStart, uint writeStart, uint dataSize);
    void copyDataToTarget(DrawIndirectBuffer &target, uint readStart, uint writeStart, uint dataSize);

    // Map and unmap the buffer to client memory.
    void* mapBuffer(uint offset, uint length, MapBufferAccess access, MapBufferSynch sych);
    void unmapBuffer();

    uint getID() { return this->bufferID; }
    bool hasData() { return this->filled; }
    uint size() const { return this->dataSize; }
  private:
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
