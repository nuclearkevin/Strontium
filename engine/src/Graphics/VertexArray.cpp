// Project includes.
#include "Graphics/VertexArray.h"

// OpenGL includes.
#include "glad/glad.h"

namespace Strontium
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
  VertexArray::addAttribute(uint location, AttribType type, bool normalized,
                            uint size, uint stride)
  {
    GLboolean glNormalized = static_cast<GLboolean>(normalized);
    this->bind();
    switch (type)
    {
      case AttribType::Vec4:
      {
        glVertexAttribPointer(location, 4, GL_FLOAT, glNormalized, size, (void*) (unsigned long) stride);
        break;
      }
      case AttribType::Vec3:
      {
        glVertexAttribPointer(location, 3, GL_FLOAT, glNormalized, size, (void*) (unsigned long) stride);
        break;
      }
      case AttribType::Vec2:
      {
        glVertexAttribPointer(location, 2, GL_FLOAT, glNormalized, size, (void*) (unsigned long) stride);
        break;
      }
      case AttribType::IVec4:
      {
        glVertexAttribIPointer(location, 4, GL_INT, size, (void*) (unsigned long) stride);
        break;
      }
      case AttribType::IVec3:
      {
        glVertexAttribIPointer(location, 3, GL_INT, size, (void*) (unsigned long) stride);
        break;
      }
      case AttribType::IVec2:
      {
        glVertexAttribIPointer(location, 2, GL_INT, size, (void*) (unsigned long) stride);
        break;
      }
    }

    glEnableVertexAttribArray(location);
  }

  // Replace the vertex buffer, also purges the index buffers for safety.
  void
  VertexArray::setData(Shared<VertexBuffer> buffer)
  {
    if (buffer != nullptr)
    {
      this->data = buffer;
      glBindVertexArray(this->arrayID);
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

  uint
  VertexArray::numToRender()
  {
    if (this->indices != nullptr)
      return this->indices->getCount();
    else
      return 0;
  }
}
