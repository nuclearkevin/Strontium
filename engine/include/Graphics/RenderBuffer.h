#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

namespace Strontium
{
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
    RenderBuffer(RenderBuffer&&) = delete;
    RenderBuffer& operator=(const RenderBuffer&) = delete;
    RenderBuffer& operator=(RenderBuffer&&) = delete;

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
}