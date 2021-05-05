// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Graphics/Buffers.h"

namespace SciRenderer
{
  class VertexArray
  {
  public:
    // Constructors and destructor.
    VertexArray(VertexBuffer* buffer);
    VertexArray(const void* bufferData, const unsigned &dataSize,
                BufferType bufferType);
    VertexArray();
    ~VertexArray();

    // Bind/unbind the array.
    void bind();
    void unbind();

    // Replace the vertex buffer, also purges the index buffers for safety.
    void setData(VertexBuffer* buffer);
    void setData(const void* bufferData, const unsigned &dataSize,
                 BufferType bufferType);

    // Add an index buffer to the vertex array.
    void addIndexBuffer(IndexBuffer* buffer);
    void addIndexBuffer(const GLuint* bufferData, unsigned numIndices,
                        BufferType bufferType);

    // Set the index buffer to be used by this vertex array.
    void setIndices(unsigned vaIndex);

    // Checks to see if the vertex array has the required objects to be rendered.
    bool canRender();

    // Return the number of vertices to render.
    unsigned numToRender();

  protected:
    // Vertex array ID.
    GLuint                    arrayID;

    // Vertex buffer with the data to be associated with this vertex array.
    VertexBuffer*             data;

    // Current index buffer for rendering.
    IndexBuffer*              indices;

    // Vector of index buffers for rendering purposes.
    std::vector<IndexBuffer*> iBuffers;
  };
}
