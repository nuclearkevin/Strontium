// Project includes.
#include "Graphics/Buffers.h"

namespace SciRenderer
{
  //----------------------------------------------------------------------------
  // Vertex buffer here.
  //----------------------------------------------------------------------------
  VertexBuffer::VertexBuffer(const void* bufferData, const unsigned &dataSize,
                             BufferType bufferType)
    : hasData(true)
    , type(bufferType)
    , bufferData(bufferData)
    , dataSize(dataSize)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
    glBufferData(GL_ARRAY_BUFFER, dataSize, bufferData, static_cast<GLuint>(bufferType));
  }

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

  //----------------------------------------------------------------------------
  // Index buffer here.
  //----------------------------------------------------------------------------
  IndexBuffer::IndexBuffer(const GLuint* bufferData,
                           unsigned numIndices,
                           BufferType bufferType)
    : hasData(true)
    , type(bufferType)
    , bufferData(bufferData)
    , count(numIndices)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->bufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * numIndices, bufferData,
                 static_cast<GLuint>(bufferType));
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
  unsigned
  IndexBuffer::getCount()
  {
    return this->count;
  }

  //----------------------------------------------------------------------------
  // Uniform buffer here.
  //----------------------------------------------------------------------------
  UniformBuffer::UniformBuffer(const unsigned &bufferSize, BufferType bufferType)
    : type(bufferType)
    , dataSize(bufferSize)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, static_cast<GLuint>(bufferType));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  UniformBuffer::UniformBuffer(const void* bufferData, const unsigned &dataSize,
                               BufferType bufferType)
    : type(bufferType)
    , dataSize(dataSize)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBufferData(GL_UNIFORM_BUFFER, dataSize, nullptr, static_cast<GLuint>(bufferType));
    glBufferSubData(GL_UNIFORM_BUFFER, 0, dataSize, bufferData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  UniformBuffer::~UniformBuffer()
  {
    glDeleteBuffers(1, &this->bufferID);
  }

  // Bind/unbind the buffer.
  void
  UniformBuffer::bindToPoint(GLuint point)
  {
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBindBufferBase(GL_UNIFORM_BUFFER, point, this->bufferID);
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

  // Set a specific part of the buffer data.
  void
  UniformBuffer::setData(GLuint start, GLuint newDataSize, const void* newData)
  {
    if (start + newDataSize > this->dataSize)
    {
      std::cout << "New data (" << newDataSize << ") at position " << start
                << " exceeds the maximum buffer size of " << this->dataSize << "."
                << std::endl;
      return;
    }
    this->bind();
    glBufferSubData(GL_UNIFORM_BUFFER, start, newDataSize, newData);
    this->unbind();
  }

  RenderBuffer::RenderBuffer(GLuint width, GLuint height)
    : format(RBOInternalFormat::DepthStencil)
  {
    glGenRenderbuffers(1, &this->bufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, this->bufferID);

    glRenderbufferStorage(GL_RENDERBUFFER,
                          static_cast<GLuint>(RBOInternalFormat::DepthStencil),
                          width, height);
  }

  RenderBuffer::RenderBuffer(GLuint width, GLuint height,
                             const RBOInternalFormat &format)
    : format(format)
  {
    glGenRenderbuffers(1, &this->bufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, this->bufferID);

    glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLuint>(format),
                          width, height);
  }

  RenderBuffer::~RenderBuffer()
  {
    glDeleteRenderbuffers(1, &this->bufferID);
  }

  void
  RenderBuffer::bind()
  {
    glBindRenderbuffer(GL_RENDERBUFFER, this->bufferID);
  }

  void
  RenderBuffer::unbind()
  {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }
}
