// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

namespace SciRenderer
{
  // Struct for framebuffer attachments.
  struct FBOAttach
  {
    GLuint id;
  };

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
    auto bind()   -> void;
    auto unbind() -> void;

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
    auto bind()     -> void;
    auto unbind()   -> void;

    // Getters.
    auto getCount() -> unsigned;

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

  // Frame buffer class.
  class FrameBuffer
  {
  public:
    // A constructor to generate a framebuffer at a particular location or any
    // location.
    FrameBuffer(GLuint width, GLuint height);
    FrameBuffer(GLuint bufferLocation, GLuint width, GLuint height);
    ~FrameBuffer();

    // Bind and unbind the framebuffer. Have to unbind before rendering to the
    // default buffer.
    void bind();
    void unbind();

    // Methods for texture/buffer generation and attachment.
    void generateColourTexture2D();
    void generateDepthTexture2D();
    void generateRenderBuffer();

    // Methods for texture attachment.
    void attachColourTexture2D(GLuint textureID, GLuint width, GLuint height);
    void attachDepthTexture2D(GLuint textureID, GLuint width, GLuint height);
    void attachRenderBuffer(GLuint bufferID, GLuint width, GLuint height);

    // Update the framebuffer size.
    void resize(GLuint width, GLuint height);

    // Get the size of the framebuffer.
    void setViewport();

    // Get the IDs of the attachments.
    GLuint getColourID();
    GLuint getDepthID();

    // Get the size of the framebuffer.
    void getSize(GLuint &outWidth, GLuint &outHeight);

    // Clear the buffer.
    void clear();

    // Check if the framebuffer is valid.
    bool isValid();

  protected:
    GLuint       bufferID;
    FBOAttach*   colourAttach;
    FBOAttach*   depthAttach;

    GLuint       width, height;

    bool         hasRenderBuffer;

    GLbitfield   clearFlags;

    glm::vec4    clearColour;
  };

  // Uniform Buffer Class.
  class UniformBuffer
  {
  public:
    // Constructors and destructor.
    UniformBuffer(const void* bufferData, const unsigned &dataSize,
                  BufferType bufferType);
    UniformBuffer(const unsigned &bufferSize, BufferType bufferType);
    ~UniformBuffer();

    // Bind/unbind the buffer.
    void bindToPoint(GLuint point);
    void bind();
    void unbind();

    // Set a specific part of the buffer data.
    void setData(GLuint start, GLuint newDataSize, const void* newData);

  protected:
    // OpenGL buffer ID.
    GLuint      bufferID;

    // If the buffer has data or not.
    bool        hasData;

    // Type of the buffer to prevent mismatching.
    BufferType  type;

    // The data currently in the buffer.
    unsigned    dataSize;
  };
}
