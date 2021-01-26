// Include guard.
#pragma once

// Project includes.
#include "Meshes.h"

// STL includes.
#include <vector>

// OpenGL includes.
#include <GL/glew.h>

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

// Builds a batch vertex buffer using a vector of meshes.
VertexBuffer* buildBatchVBuffer(const std::vector<Mesh*> &loadedMeshes,
                                BufferType bufferType);

// Builds a batch index buffer using a vector of meshes.
IndexBuffer* buildBatchIBuffer(const std::vector<Mesh*> &loadedMeshes,
                               BufferType bufferType);
