#include "Graphics/RenderBuffer.h"

// OpenGL includes.
#include "glad/glad.h"

namespace Strontium
{
RenderBuffer::RenderBuffer()
    : format(RBOInternalFormat::DepthStencil)
    , width(0)
    , height(0)
  {
    glGenRenderbuffers(1, &this->bufferID);
  }

  RenderBuffer::RenderBuffer(uint width, uint height)
    : format(RBOInternalFormat::DepthStencil)
    , width(width)
    , height(height)
  {
    glGenRenderbuffers(1, &this->bufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, this->bufferID);

    glRenderbufferStorage(GL_RENDERBUFFER,
                          static_cast<GLenum>(RBOInternalFormat::DepthStencil),
                          width, height);
  }

  RenderBuffer::RenderBuffer(uint width, uint height,
                             const RBOInternalFormat &format)
    : format(format)
    , width(width)
    , height(height)
  {
    glGenRenderbuffers(1, &this->bufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, this->bufferID);

    glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLenum>(format),
                          width, height);
  }

  RenderBuffer::~RenderBuffer()
  {
    glDeleteRenderbuffers(1, &this->bufferID);
  }

  void 
  RenderBuffer::reset(uint newWidth, uint newHeight)
  {
    this->width = newWidth;
    this->height = newHeight;
    glBindRenderbuffer(GL_RENDERBUFFER, this->bufferID);

    glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLenum>(this->format),
                          this->width, this->height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }

  void 
  RenderBuffer::reset(uint newWidth, uint newHeight, const RBOInternalFormat& newFormat)
  {
    this->format = newFormat;
    this->width = newWidth;
    this->height = newHeight;
    glBindRenderbuffer(GL_RENDERBUFFER, this->bufferID);

    glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLenum>(this->format),
                          this->width, this->height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
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