// Project includes.
#include "Graphics/Buffers.h"

// OpenGL includes.
#include "glad/glad.h"

namespace Strontium
{
  //----------------------------------------------------------------------------
  // Vertex buffer here.
  //----------------------------------------------------------------------------
  VertexBuffer::VertexBuffer(const void* bufferData, uint dataSize,
                             BufferType bufferType)
    : hasData(true)
    , type(bufferType)
    , dataSize(dataSize)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glBufferData(GL_ARRAY_BUFFER, dataSize, bufferData, static_cast<GLenum>(bufferType));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  VertexBuffer::VertexBuffer(BufferType bufferType)
    : hasData(false)
    , type(bufferType)
    , dataSize(0u)
  { }

  VertexBuffer::~VertexBuffer()
  {
    glDeleteBuffers(1, &this->bufferID);
  }

  // Bind the buffer.
  void
  VertexBuffer::bind()
  {
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
  }

  // Unbind the buffer.
  void
  VertexBuffer::unbind()
  {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  // Resize the buffer.
  void 
  VertexBuffer::resize(uint bufferSize, BufferType bufferType)
  {
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, static_cast<GLenum>(bufferType));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    this->dataSize = bufferSize;
  }

  // Modify the buffer's contents.
  void
  VertexBuffer::setData(uint start, uint newDataSize, const void* newData)
  {
    assert(("New data exceeds buffer size.", !(start + newDataSize > this->dataSize)));

    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glBufferSubData(GL_ARRAY_BUFFER, start, newDataSize, newData);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    this->hasData = true;
  }

  //----------------------------------------------------------------------------
  // Index buffer here.
  //----------------------------------------------------------------------------
  IndexBuffer::IndexBuffer(const uint* bufferData,
                           uint numIndices,
                           BufferType bufferType)
    : hasData(true)
    , type(bufferType)
    , count(numIndices)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->bufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * numIndices, bufferData,
                 static_cast<GLenum>(bufferType));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  IndexBuffer::~IndexBuffer()
  {
    glDeleteBuffers(1, &this->bufferID);
  }

  // Bind the buffer.
  void
  IndexBuffer::bind()
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->bufferID);
  }

  // Unbind the buffer.
  void
  IndexBuffer::unbind()
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  // Get the number of stored vertices.
  uint
  IndexBuffer::getCount()
  {
    return this->count;
  }

  //----------------------------------------------------------------------------
  // Uniform buffer here.
  //----------------------------------------------------------------------------
  UniformBuffer::UniformBuffer(uint bufferSize, BufferType bufferType)
    : type(bufferType)
    , dataSize(bufferSize)
    , filled(false)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, static_cast<GLenum>(bufferType));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  UniformBuffer::UniformBuffer(const void* bufferData, uint dataSize,
                               BufferType bufferType)
    : type(bufferType)
    , dataSize(dataSize)
    , filled(true)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBufferData(GL_UNIFORM_BUFFER, dataSize, bufferData, static_cast<GLenum>(bufferType));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  UniformBuffer::UniformBuffer()
    : type(BufferType::Static)
    , dataSize(0)
    , filled(false)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  UniformBuffer::~UniformBuffer()
  {
    glDeleteBuffers(1, &this->bufferID);
  }

  // Bind/unbind the buffer.
  void
  UniformBuffer::bindToPoint(const uint bindPoint)
  {
    glBindBufferBase(GL_UNIFORM_BUFFER, bindPoint, this->bufferID);
  }

  void
  UniformBuffer::bind()
  {
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
  }

  void
  UniformBuffer::unbind()
  {
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  // Resize the buffer.
  void 
  UniformBuffer::resize(uint bufferSize, BufferType bufferType)
  {
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, static_cast<GLenum>(bufferType));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    this->dataSize = bufferSize;
  }

  // Set a specific part of the buffer data.
  void
  UniformBuffer::setData(uint start, uint newDataSize, const void* newData)
  {
    assert(("New data exceeds buffer size.", !(start + newDataSize > this->dataSize)));

    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBufferSubData(GL_UNIFORM_BUFFER, start, newDataSize, newData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    this->filled = true;
  }

  //----------------------------------------------------------------------------
  // Shader storage buffer here.
  //----------------------------------------------------------------------------
  ShaderStorageBuffer::ShaderStorageBuffer(const void* bufferData, uint dataSize,
                                           BufferType bufferType)
    : filled(true)
    , type(bufferType)
    , dataSize(dataSize)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufferID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, bufferData, static_cast<GLenum>(bufferType));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  }

  ShaderStorageBuffer::ShaderStorageBuffer(uint bufferSize, BufferType bufferType)
    : filled(false)
    , type(bufferType)
    , dataSize(bufferSize)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufferID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, static_cast<GLenum>(bufferType));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  }

  ShaderStorageBuffer::~ShaderStorageBuffer()
  {
    glDeleteBuffers(1, &this->bufferID);
  }

  void
  ShaderStorageBuffer::bind()
  {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufferID);
  }

  void
  ShaderStorageBuffer::bindToPoint(const uint bindPoint)
  {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint, this->bufferID);
  }

  void
  ShaderStorageBuffer::unbind()
  {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  }

  // Resize the buffer.
  void 
  ShaderStorageBuffer::resize(uint bufferSize, BufferType bufferType)
  {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufferID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, static_cast<GLenum>(bufferType));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    this->dataSize = bufferSize;
  }

  void
  ShaderStorageBuffer::setData(uint start, uint newDataSize,
                               const void* newData)
  {
    assert(("New data exceeds buffer size.", !(start + newDataSize > this->dataSize)));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufferID);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, start, newDataSize, newData);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    this->filled = true;
  }

  void* 
  ShaderStorageBuffer::mapBuffer(uint offset, uint length, MapBufferAccess access, 
                                 MapBufferSynch sych)
  {
    return glMapNamedBufferRange(this->bufferID, offset, length, static_cast<GLbitfield>(access) | static_cast<GLbitfield>(sych));
  }

  void 
  ShaderStorageBuffer::unmapBuffer()
  {
    glUnmapNamedBuffer(this->bufferID);
  }
}
