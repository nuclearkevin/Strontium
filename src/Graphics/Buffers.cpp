// Project includes.
#include "Graphics/Buffers.h"

namespace SciRenderer
{
  /**
   *  Vertex buffer starts here.
   */
  // Constructor and destructor.
  VertexBuffer::VertexBuffer(const void* bufferData, const unsigned &dataSize,
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

  /**
   *  Index buffer starts here.
   */
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * numIndices, bufferData,
                 bufferType);
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

  /**
   *------------------------------------------------------------------------------
   *  Framebuffer starts here. TODO: finish it.
   *------------------------------------------------------------------------------
   */
  // Constructors and destructor.
  FrameBuffer::FrameBuffer(GLuint width, GLuint height)
    : width(width)
    , height(height)
    , hasRenderBuffer(false)
    , clearFlags(0)
    , clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
  {
    glGenFramebuffers(1, &this->bufferID);
  }

  FrameBuffer::FrameBuffer(GLuint bufferLocation, GLuint width, GLuint height)
    : width(width)
    , height(height)
    , hasRenderBuffer(false)
    , clearFlags(0)
    , clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
  {
    this->bufferID = bufferLocation;
    glGenFramebuffers(1, &this->bufferID);
  }

  FrameBuffer::~FrameBuffer()
  {
    // Actual buffer delete.
    glDeleteFramebuffers(1, &this->bufferID);
  }

  // Bind the buffer.
  void
  FrameBuffer::bind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, this->bufferID);
  }

  // Unbind the buffer.
  void
  FrameBuffer::unbind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // Methods for managing framebuffer textures and render buffers..
  void
  FrameBuffer::generateColourTexture2D()
  {
    if (this->colourAttach == nullptr)
      this->colourAttach = new FBOAttach();
    else
      return;

    this->bind();
    glGenTextures(1, &this->colourAttach->id);
    glBindTexture(GL_TEXTURE_2D, this->colourAttach->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->width, this->height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           this->colourAttach->id, 0);
    this->unbind();
    this->clearFlags |= GL_COLOR_BUFFER_BIT;
  }

  void
  FrameBuffer::generateDepthTexture2D()
  {
    if (this->depthAttach == nullptr)
      this->depthAttach = new FBOAttach();
    else
      return;

    this->bind();
    glGenTextures(1, &this->depthAttach->id);
    glBindTexture(GL_TEXTURE_2D, this->depthAttach->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->width, this->height,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           this->depthAttach->id, 0);
    this->unbind();
    this->clearFlags |= GL_DEPTH_BUFFER_BIT;
  }

  void
  FrameBuffer::generateRenderBuffer()
  {
    if (this->depthAttach == nullptr)
      this->depthAttach = new FBOAttach();
    else
      return;

    this->bind();
    glGenRenderbuffers(1, &this->depthAttach->id);
    glBindRenderbuffer(GL_RENDERBUFFER, this->depthAttach->id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->width, this->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, this->depthAttach->id);
    this->unbind();
    this->clearFlags |= GL_DEPTH_BUFFER_BIT;
    this->hasRenderBuffer = true;
  }

  // Attach a pre-generated texture or renderbuffer to the FBO.
  void
  FrameBuffer::attachColourTexture2D(GLuint textureID, GLuint width, GLuint height)
  {
    // Overwrites the current colour texture.
    if (this->colourAttach != nullptr)
    {
      glDeleteTextures(1, &this->colourAttach->id);
      delete this->colourAttach;
      this->colourAttach = new FBOAttach();
    }
    else
    {
      this->colourAttach = new FBOAttach();
    }

    this->colourAttach->id = textureID;
  }

  void
  FrameBuffer::attachDepthTexture2D(GLuint textureID, GLuint width, GLuint height)
  {
    // Overwrites the current depth texture.
    if (this->depthAttach != nullptr)
    {
      glDeleteTextures(1, &this->depthAttach->id);
      delete this->depthAttach;
      this->depthAttach = new FBOAttach();
    }
    else
    {
      this->depthAttach = new FBOAttach();
    }

    this->depthAttach->id = textureID;
  }

  void
  FrameBuffer::attachRenderBuffer(GLuint bufferID, GLuint width, GLuint height)
  {
    // Overwrites the current renderbuffer.
    if (this->depthAttach != nullptr)
    {
      glDeleteRenderbuffers(1, &this->depthAttach->id);
      delete this->depthAttach;
      this->depthAttach = new FBOAttach();
    }
    else
    {
      this->depthAttach = new FBOAttach();
    }

    this->depthAttach->id = bufferID;
  }

  // Resize the framebuffer.
  void
  FrameBuffer::resize(GLuint width, GLuint height)
  {
    this->width = width;
    this->height = height;

    if (this->colourAttach != nullptr)
    {
      // Update the colour attachment texture.
      this->bind();
      glBindTexture(GL_TEXTURE_2D, this->colourAttach->id);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->width, this->height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             this->colourAttach->id, 0);
      this->unbind();
    }
    if (this->depthAttach != nullptr && hasRenderBuffer)
    {
      // Update the depth renderbuffer.
      this->bind();

      glBindRenderbuffer(GL_RENDERBUFFER, this->depthAttach->id);

      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->width,
                            this->height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_RENDERBUFFER, this->depthAttach->id);
      this->unbind();
    }
    if (this->depthAttach != nullptr && !hasRenderBuffer)
    {
      // Update the depth attachment texture. TODO.

    }
  }

  void
  FrameBuffer::setViewport()
  {
    glViewport(0, 0, this->width, this->height);
  }

  // Fetch the texture IDs.
  GLuint
  FrameBuffer::getColourID()
  {
    if (this->colourAttach != nullptr)
      return this->colourAttach->id;
    else
      return 0;
  }

  GLuint
  FrameBuffer::getDepthID()
  {
    if (this->depthAttach != nullptr && !this->hasRenderBuffer)
      return this->depthAttach->id;
    else
      return 0;
  }

  void
  FrameBuffer::getSize(GLuint &outWidth, GLuint &outHeight)
  {
    outWidth = this->width;
    outHeight = this->height;
  }

  void
  FrameBuffer::clear()
  {
    this->bind();
    glClearColor(this->clearColour[0], this->clearColour[1], this->clearColour[2],
                 this->clearColour[3]);
    glClear(this->clearFlags);
    this->unbind();
  }

  bool
  FrameBuffer::isValid()
  {
    this->bind();
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cout << "The framebuffer is not valid for a drawcall." << std::endl;
      return false;
    }
    else
    {
      return true;
    }
    this->unbind();
  }

  /**
   *------------------------------------------------------------------------------
   *  Uniform buffer starts here.
   *------------------------------------------------------------------------------
   */
  UniformBuffer::UniformBuffer(const unsigned &bufferSize, BufferType bufferType)
    : type(bufferType)
    , dataSize(bufferSize)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, bufferType);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  UniformBuffer::UniformBuffer(const void* bufferData, const unsigned &dataSize,
                               BufferType bufferType)
    : type(bufferType)
    , dataSize(dataSize)
  {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, this->bufferID);
    glBufferData(GL_UNIFORM_BUFFER, dataSize, nullptr, bufferType);
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
}