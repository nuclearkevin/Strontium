#include "Graphics/FrameBuffer.h"

// Project includes.
#include "Core/Logs.h"

namespace SciRenderer
{
  FrameBuffer::FrameBuffer()
    : depthBuffer(nullptr)
    , width(0)
    , height(0)
    , hasRenderBuffer(false)
    , clearFlags(0)
    , clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
  {
    glGenFramebuffers(1, &this->bufferID);
  }

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
    // Delete the texture attachments.
    for (auto& pair : this->textureAttachments)
    {
      auto tex = pair.second.second;

      if (tex == nullptr)
        continue;

      Textures::deleteTexture(tex);
    }

    if (this->depthBuffer != nullptr)
      delete this->depthBuffer;

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
  FrameBuffer::attachTexture2D(const FBOSpecification &spec, const bool &removeTex)
  {
    Logger* logs = Logger::getInstance();

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

    this->bind();

    // If an attachment target already exists, remove it from the map and add
    // the new target.
    auto targetLoc = this->textureAttachments.find(spec.target);
    if (targetLoc != this->textureAttachments.end())
    {
      if (removeTex)
        Textures::deleteTexture(targetLoc->second.second);
      this->textureAttachments.erase(targetLoc);
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

    // Setup the clear flags.
    if (spec.target == FBOTargetParam::Depth)
      this->clearFlags |= GL_DEPTH_BUFFER_BIT;
    else if (spec.target == FBOTargetParam::Stencil)
      this->clearFlags |=GL_STENCIL_BUFFER_BIT;
    else if (spec.target == FBOTargetParam::DepthStencil)
      this->clearFlags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    else
      this->clearFlags |= GL_COLOR_BUFFER_BIT;

    this->textureAttachments.insert({ spec.target, { spec, newTex } });
  }

  void
  FrameBuffer::attachTexture2D(const FBOSpecification &spec, Texture2D* tex,
                               const bool &removeTex)
  {
    Logger* logs = Logger::getInstance();

    if (tex->width != this->width || tex->height != this->height)
    {
      std::cout << "Cannot attach this texture, dimensions do not agree!" << std::endl;
      return;
    }

    this->bind();

    // If an attachment target already exists, remove it from the map and add
    // the new target.
    auto targetLoc = this->textureAttachments.find(spec.target);
    if (targetLoc != this->textureAttachments.end())
    {
      if (removeTex)
        Textures::deleteTexture(targetLoc->second.second);
      this->textureAttachments.erase(targetLoc);
    }


    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<GLuint>(spec.target),
                           static_cast<GLuint>(spec.type),
                           tex->textureID, 0);

    this->unbind();

    this->clearFlags |= GL_COLOR_BUFFER_BIT;

    this->textureAttachments.insert({ spec.target, { spec, tex } });
  }

  void
  FrameBuffer::attachCubeMapFace(const FBOSpecification &spec, CubeMap* map,
                                 const bool &removeTex, GLuint mip)
  {
    Logger* logs = Logger::getInstance();

    // Error check to make sure that:
    // a) The attachment type is a cubemap
    // b) THe dimensions agree.
    GLuint faceID = static_cast<GLuint>(spec.type) -
                    static_cast<GLuint>(FBOTex2DParam::CubeMapPX);
    if (faceID < 0 || faceID > 5)
    {
      std::cout << "Cannot attach, face ID out of bounds!" << std::endl;
      return;
    }

    if (map->width[faceID] != (GLuint) ((GLfloat) this->width / std::pow(0.5f, mip))
        || map->height[faceID] != (GLuint) ((GLfloat) this->height / std::pow(0.5f, mip)))
    {
      std::cout << "Cannot attach this cubemap face, dimensions do not agree!"
                << std::endl;
      return;
    }
    this->bind();

    // If an attachment target already exists, remove it from the map and add
    // the new target.
    auto targetLoc = this->textureAttachments.find(spec.target);
    if (targetLoc != this->textureAttachments.end())
    {
      if (removeTex)
        Textures::deleteTexture(targetLoc->second.second);
      this->textureAttachments.erase(targetLoc);
    }

    Texture2D* tex = new Texture2D();
    tex->textureID = map->textureID;
    tex->width = this->width;
    tex->height = this->height;

    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<GLuint>(spec.target),
                           static_cast<GLuint>(spec.type),
                           tex->textureID, mip);
    this->unbind();

    this->clearFlags |= GL_COLOR_BUFFER_BIT;

    this->textureAttachments.insert({ spec.target, { spec, tex } });
  }

  void
  FrameBuffer::attachRenderBuffer()
  {
    Logger* logs = Logger::getInstance();

    if (this->depthBuffer != nullptr)
    {
      std::cout << "This framebuffer already has a render buffer" << std::endl;
      return;
    }
    this->bind();

    this->depthBuffer = new RenderBuffer(this->width, this->height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, this->depthBuffer->getID());
    this->unbind();

    this->clearFlags |= GL_DEPTH_BUFFER_BIT;
    this->hasRenderBuffer = true;
  }

  void
  FrameBuffer::attachRenderBuffer(RenderBuffer* buffer)
  {
    Logger* logs = Logger::getInstance();

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

  // Unattach the texture and return it (if it exists).
  Texture2D*
  FrameBuffer::unattachTexture2D(const FBOTargetParam &attachment)
  {
    auto targetLoc = this->textureAttachments.find(attachment);
    if (targetLoc != this->textureAttachments.end())
    {
      auto outTex = this->textureAttachments[attachment].second;
      this->textureAttachments.erase(targetLoc);
      return outTex;
    }
    else
    {
      return nullptr;
    }
  }

  void
  FrameBuffer::setDrawBuffers()
  {
    this->bind();
    std::vector<GLenum> totalAttachments;
    for (auto& pair : this->textureAttachments)
    {
      if (pair.first != FBOTargetParam::Depth && pair.first != FBOTargetParam::Stencil
          && pair.first != FBOTargetParam::DepthStencil)
        totalAttachments.push_back(static_cast<GLenum>(pair.first));
    }
    if (totalAttachments.size() >= 1)
      glDrawBuffers(totalAttachments.size(), &totalAttachments[0]);
    this->unbind();
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
  FBOCommands::getDefaultColourSpec(const FBOTargetParam &attach)
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
  FBOCommands::getFloatColourSpec(const FBOTargetParam &attach)
  {
    FBOSpecification floatColour = FBOSpecification();
    floatColour.target = attach;
    floatColour.type = FBOTex2DParam::Texture2D;
    floatColour.internal = TextureInternalFormats::RGBA16f;
    floatColour.format = TextureFormats::RGBA;
    floatColour.dataType = TextureDataType::Floats;
    floatColour.sWrap = TextureWrapParams::Repeat;
    floatColour.tWrap = TextureWrapParams::Repeat;
    floatColour.minFilter = TextureMinFilterParams::Linear;
    floatColour.maxFilter = TextureMaxFilterParams::Linear;

    return floatColour;
  }

  FBOSpecification
  FBOCommands::getDefaultDepthSpec()
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
