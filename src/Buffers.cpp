// Project includes.
#include "Buffers.h"

using namespace SciRenderer;

// Constructor and destructor.
VertexBuffer::VertexBuffer(const void* bufferData,
                           const unsigned &dataSize,
                           BufferType bufferType)
  : hasData(true)
  , type(bufferType)
  , bufferData(bufferData)
  , dataSize(dataSize)
{
  glGenBuffers(1, &this->bufferID);
  glBindBuffer(GL_ARRAY_BUFFER, this->bufferID);
  glBufferData(GL_ARRAY_BUFFER, dataSize, bufferData, bufferType);
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

// Constructor and destructor.
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
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * numIndices, bufferData, bufferType);
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

// Build a vertex buffer with a vector of meshes.
VertexBuffer*
SciRenderer::buildBatchVBuffer(const std::vector<Mesh*> &loadedMeshes,
                               BufferType bufferType)
{
  // Vector for merging all the vertex data.
  std::vector<Vertex> batchData;

  for (auto &mesh : loadedMeshes)
  {
    for (auto &vertex : mesh->getData())
    {
      batchData.push_back(vertex);
    }
  }
  return new VertexBuffer(&(batchData[0]), batchData.size() * sizeof(Vertex),
                          bufferType);
}

// Build an index buffer with a vector of meshes.
IndexBuffer*
SciRenderer::buildBatchIBuffer(const std::vector<Mesh*> &loadedMeshes,
                               BufferType bufferType)
{
  // Vector for merging all the indices.
  std::vector<GLuint> batchIndices;

  // Vertex offset.
  unsigned offset = 0;

  for (auto &mesh : loadedMeshes)
  {
    for (auto &index : mesh->getIndices())
    {
      batchIndices.push_back(index + offset);
    }
    offset += mesh->getData().size();
  }
  return new IndexBuffer(&(batchIndices[0]), batchIndices.size(), bufferType);
}
