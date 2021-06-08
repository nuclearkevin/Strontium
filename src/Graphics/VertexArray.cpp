// Project includes.
#include "Graphics/VertexArray.h"

namespace SciRenderer
{
  // Constructors and destructors.
  VertexArray::VertexArray(Shared<VertexBuffer> buffer)
    : data(buffer)
    , indices(nullptr)
  {
    glGenVertexArrays(1, &this->arrayID);
  	glBindVertexArray(this->arrayID);
    this->data->bind();
  }

  VertexArray::VertexArray(const void* bufferData, const unsigned &dataSize,
                           BufferType bufferType)
  {
    this->data = createShared<VertexBuffer>(bufferData, dataSize, bufferType);
    glGenVertexArrays(1, &this->arrayID);
  	glBindVertexArray(this->arrayID);
    this->data->bind();
  }

  VertexArray::VertexArray()
    : data(nullptr)
    , indices(nullptr)
  {
    glGenVertexArrays(1, &this->arrayID);
  	glBindVertexArray(this->arrayID);
  }

  VertexArray::~VertexArray()
  {
    glDeleteVertexArrays(1, &this->arrayID);
  }

  // Bind the vertex array.
  void
  VertexArray::bind()
  {
    glBindVertexArray(this->arrayID);
    if (this->data != nullptr)
      this->data->bind();
    if (this->indices != nullptr)
      this->indices->bind();
  }

  // Unbind the vertex array.
  void
  VertexArray::unbind()
  {
    glBindVertexArray(0);
    if (this->data != nullptr)
      this->data->unbind();
    if (this->indices != nullptr)
      this->indices->unbind();
  }

  // Add an attribute
  void
  VertexArray::addAttribute(GLuint location, AttribType type, GLboolean normalized,
                            unsigned size, unsigned stride)
  {
    this->bind();
    glVertexAttribPointer(location, static_cast<GLint>(type), GL_FLOAT,
													normalized, size, (void*) (unsigned long) stride);
		glEnableVertexAttribArray(location);
  }

  // Replace the vertex buffer, also purges the index buffers for safety.
  void
  VertexArray::setData(Shared<VertexBuffer> buffer)
  {
    if (buffer != nullptr)
    {
      this->data = buffer;
      this->data->bind();
      this->indices = nullptr;
      this->iBuffers.clear();
    }
  }

  void
  VertexArray::setData(const void* bufferData, const unsigned &dataSize,
                       BufferType bufferType)
  {
    this->data.reset(new VertexBuffer(bufferData, dataSize, bufferType));
    this->data->bind();
  }

  // Add an index buffer with its pointer.
  void
  VertexArray::addIndexBuffer(Shared<IndexBuffer> buffer)
  {
    if (buffer != nullptr)
      this->iBuffers.push_back(buffer);
    if (this->iBuffers.size() == 1 && buffer != nullptr)
    {
      this->indices = buffer;
      this->indices->bind();
    }
  }

  // Construct and add an index buffer.
  void
  VertexArray::addIndexBuffer(const GLuint* bufferData, unsigned numIndices,
                              BufferType bufferType)
  {
    Shared<IndexBuffer> buffer = createShared<IndexBuffer>(bufferData, numIndices, bufferType);
    if (buffer != nullptr)
      this->iBuffers.push_back(buffer);
    if (this->iBuffers.size() == 1 && buffer != nullptr)
    {
      this->indices = buffer;
      this->indices->bind();
    }
  }

  // Set the current index buffer (used while rendering).
  void
  VertexArray::setIndices(unsigned vaIndex)
  {
    if (vaIndex < this->iBuffers.size())
    {
      this->indices = this->iBuffers[vaIndex];
      this->indices->bind();
    }
  }

  // See if the vertex array has everything needed for a render.
  bool
  VertexArray::canRender()
  {
    if (this->indices != nullptr && this->data != nullptr)
      return true;
    else
      return false;
  }

  unsigned
  VertexArray::numToRender()
  {
    if (this->indices != nullptr)
      return this->indices->getCount();
    else
      return 0;
  }
}
