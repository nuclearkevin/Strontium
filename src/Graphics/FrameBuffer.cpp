#include "Graphics/FrameBuffer.h"

// Project includes.
#include "Core/Logs.h"

namespace SciRenderer
{
  // Constructors and destructor.
  FrameBuffer::FrameBuffer(GLuint width, GLuint height)
    : depthBuffer(nullptr)
    , width(width)
    , height(height)
    , hasRenderBuffer(false)
    , clearFlags(0)
    , clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
  {
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

  // Attach a texture to the framebuffer.
  void
  FrameBuffer::attachTexture2D(const FBOSpecification &spec)
  {
    Logger* logs = Logger::getInstance();

    this->bind();
    Texture2D* newTex = new Texture2D();

    newTex->width = this->width;
    newTex->height = this->height;

    if (spec.format == TextureFormats::Red || spec.format == TextureFormats::Depth)
      newTex->n = 1;
    else if (spec.format == TextureFormats::RG || spec.format == TextureFormats::DepthStencil)
      newTex->n = 2;
    else if (spec.format == TextureFormats::RGB)
      newTex->n = 3;
    else if (spec.format == TextureFormats::RGBA)
      newTex->n = 4;
    else
    {
      std::cout << "Unknown format, failed to attach." << std::endl;
      delete newTex;
      return;
    }

    glGenTextures(1, &newTex->textureID);
    glBindTexture(static_cast<GLuint>(spec.type), newTex->textureID);
    glTexImage2D(static_cast<GLuint>(spec.type), 0,
                 static_cast<GLuint>(spec.internal), this->width, this->height, 0,
                 static_cast<GLuint>(spec.format),
                 static_cast<GLuint>(spec.dataType), nullptr);

    glTexParameteri(static_cast<GLuint>(spec.type), GL_TEXTURE_WRAP_S,
                    static_cast<GLuint>(spec.sWrap));
    glTexParameteri(static_cast<GLuint>(spec.type), GL_TEXTURE_WRAP_T,
                    static_cast<GLuint>(spec.tWrap));
    glTexParameteri(static_cast<GLuint>(spec.type), GL_TEXTURE_MIN_FILTER,
                    static_cast<GLuint>(spec.minFilter));
    glTexParameteri(static_cast<GLuint>(spec.type), GL_TEXTURE_MAG_FILTER,
                    static_cast<GLuint>(spec.maxFilter));

    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<GLuint>(spec.target),
                           static_cast<GLuint>(spec.type),
                           newTex->textureID, 0);
    this->unbind();

    this->clearFlags |= GL_COLOR_BUFFER_BIT;

    this->textureAttachments.insert({ spec.target, { spec, newTex } });
    this->currentAttachments.push_back(spec.target);
  }

  void
  FrameBuffer::attachTexture2D(const FBOSpecification &spec, Texture2D* tex)
  {
    Logger* logs = Logger::getInstance();

    if (tex->width != this->width || tex->height != this->height)
    {
      std::cout << "Cannot attach this texture, dimensions do not agree!" << std::endl;
      return;
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<GLuint>(spec.target),
                           static_cast<GLuint>(spec.type),
                           tex->textureID, 0);

    this->textureAttachments.insert({ spec.target, { spec, tex } });
    this->currentAttachments.push_back(spec.target);
  }

  void
  FrameBuffer::attachRenderBuffer()
  {
    if (this->depthBuffer != nullptr)
    {
      std::cout << "This framebuffer already has a render buffer" << std::endl;
      return;
    }

    this->depthBuffer = new RenderBuffer(this->width, this->height);

    this->bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, this->depthBuffer->getID());
    this->unbind();
    this->clearFlags |= GL_DEPTH_BUFFER_BIT;
    this->hasRenderBuffer = true;
  }

  void
  FrameBuffer::attachRenderBuffer(RenderBuffer* buffer)
  {
    if (this->depthBuffer != nullptr)
    {
      std::cout << "This framebuffer already has a render buffer" << std::endl;
      return;
    }

    this->bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, buffer->getID());
    this->unbind();
    this->depthBuffer = buffer;
    this->clearFlags |= GL_DEPTH_BUFFER_BIT;
    this->hasRenderBuffer = true;
  }

  // Resize the framebuffer.
  void
  FrameBuffer::resize(GLuint width, GLuint height)
  {
    this->width = width;
    this->height = height;

    // Update the attached textures.
    for (auto& pair : this->textureAttachments)
    {
      auto spec = pair.second.first;
      auto tex = pair.second.second;

      if (tex == nullptr)
        continue;

      Textures::bindTexture(tex);
      glTexImage2D(static_cast<GLuint>(spec.type), 0,
                   static_cast<GLuint>(spec.internal), this->width, this->height, 0,
                   static_cast<GLuint>(spec.format),
                   static_cast<GLuint>(spec.dataType), nullptr);
    }

    // Update the attached render buffer.
    if (this->depthBuffer != nullptr)
    {
      this->depthBuffer->bind();
      glRenderbufferStorage(GL_RENDERBUFFER,
                            static_cast<GLuint>(this->depthBuffer->getFormat()),
                            this->width, this->height);
    }
  }

  void
  FrameBuffer::setViewport()
  {
    glViewport(0, 0, this->width, this->height);
  }

  GLuint
  FrameBuffer::getRenderBufferID()
  {
    if (this->depthBuffer != nullptr)
      return this->depthBuffer->getID();
    else
      return 0;
  }

  GLuint
  FrameBuffer::getAttachID(const FBOTargetParam &attachment)
  {
    return this->textureAttachments.at(attachment).second->textureID;
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

  FBOSpecification
  FrameBufferCommands::getDefaultColourSpec(const FBOTargetParam &attach)
  {
    FBOSpecification defaultColour = FBOSpecification();
    defaultColour.target = attach;
    defaultColour.type = FBOTex2DParam::Texture2D;
    defaultColour.internal = TextureInternalFormats::RGB;
    defaultColour.format = TextureFormats::RGB;
    defaultColour.dataType = TextureDataType::Bytes;
    defaultColour.sWrap = TextureWrapParams::Repeat;
    defaultColour.tWrap = TextureWrapParams::Repeat;
    defaultColour.minFilter = TextureMinFilterParams::Linear;
    defaultColour.maxFilter = TextureMaxFilterParams::Linear;

    return defaultColour;
  }

  FBOSpecification
  FrameBufferCommands::getFloatColourSpec(const FBOTargetParam &attach)
  {
    FBOSpecification floatColour = FBOSpecification();
    floatColour.target = attach;
    floatColour.type = FBOTex2DParam::Texture2D;
    floatColour.internal = TextureInternalFormats::RGB16f;
    floatColour.format = TextureFormats::RGB;
    floatColour.dataType = TextureDataType::Floats;
    floatColour.sWrap = TextureWrapParams::Repeat;
    floatColour.tWrap = TextureWrapParams::Repeat;
    floatColour.minFilter = TextureMinFilterParams::Linear;
    floatColour.maxFilter = TextureMaxFilterParams::Linear;

    return floatColour;
  }

  FBOSpecification
  FrameBufferCommands::getDefaultDepthSpec()
  {
    FBOSpecification defaultDepth = FBOSpecification();
    defaultDepth.target = FBOTargetParam::DepthStencil;
    defaultDepth.type = FBOTex2DParam::Texture2D;
    defaultDepth.internal = TextureInternalFormats::Depth24Stencil8;
    defaultDepth.format = TextureFormats::DepthStencil;
    defaultDepth.dataType = TextureDataType::UInt24UInt8;
    defaultDepth.sWrap = TextureWrapParams::Repeat;
    defaultDepth.tWrap = TextureWrapParams::Repeat;
    defaultDepth.minFilter = TextureMinFilterParams::Linear;
    defaultDepth.maxFilter = TextureMaxFilterParams::Linear;

    return defaultDepth;
  }
}
