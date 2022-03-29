#include "Graphics/FrameBuffer.h"

// Project includes.
#include "Core/Logs.h"

// OpenGL includes.
#include "glad/glad.h"

namespace Strontium
{
  FrameBuffer::FrameBuffer()
    : depthBuffer()
    , width(1)
    , height(1)
    , hasRenderBuffer(false)
    , clearFlags(0)
    , clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
  {
    glGenFramebuffers(1, &this->bufferID);
  }

  // Constructors and destructor.
  FrameBuffer::FrameBuffer(uint width, uint height)
    : depthBuffer()
    , width(width)
    , height(height)
    , hasRenderBuffer(false)
    , clearFlags(0)
    , clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
  {
    glGenFramebuffers(1, &this->bufferID);
    assert(((width != 0 && height != 0), "Framebuffer width and height cannot be zero."));
  }

  FrameBuffer::~FrameBuffer()
  {
    // Delete the texture attachments.
    for (auto& attachments : this->textureAttachments)
      if (attachments.second.ownedByFBO)
        glDeleteTextures(1, &attachments.second.attachmentID);
    this->textureAttachments.clear();

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

  void 
  FrameBuffer::attach(const Texture2DParams& params, const FBOAttachment& attachment)
  {
    auto targetLoc = this->textureAttachments.find(attachment.target);
    if (targetLoc != this->textureAttachments.end())
    {
      if (targetLoc->second.ownedByFBO)
        glDeleteTextures(1, &targetLoc->second.attachmentID);
      this->textureAttachments.erase(targetLoc);
    }

    this->textureAttachments.emplace(attachment.target, attachment);
    auto& ownedAttachment = textureAttachments.at(attachment.target);

    if (ownedAttachment.ownedByFBO && ownedAttachment.attachmentID == 0)
    {
      // Generate a 2D texture if it is supposed to be owned by an FBO.
      glGenTextures(1, &ownedAttachment.attachmentID);
      glBindTexture(GL_TEXTURE_2D, ownedAttachment.attachmentID);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                      static_cast<GLenum>(params.sWrap));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                      static_cast<GLenum>(params.tWrap));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      static_cast<GLenum>(params.minFilter));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                      static_cast<GLenum>(params.maxFilter));

      glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(ownedAttachment.internal),
          this->width, this->height, 0, static_cast<GLenum>(ownedAttachment.format),
          static_cast<GLenum>(ownedAttachment.dataType), nullptr);
      glBindTexture(GL_TEXTURE_2D, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, this->bufferID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<uint>(ownedAttachment.target), 
                           static_cast<uint>(ownedAttachment.type), ownedAttachment.attachmentID, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Setup the clear flags.
    if (ownedAttachment.target == FBOTargetParam::Depth)
        this->clearFlags |= GL_DEPTH_BUFFER_BIT;
    else if (ownedAttachment.target == FBOTargetParam::Stencil)
        this->clearFlags |= GL_STENCIL_BUFFER_BIT;
    else if (ownedAttachment.target == FBOTargetParam::DepthStencil)
        this->clearFlags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    else
        this->clearFlags |= GL_COLOR_BUFFER_BIT;
  }

  void
  FrameBuffer::attachRenderBuffer(RBOInternalFormat format)
  {
    this->depthBuffer.reset(this->width, this->height, format);
    this->bind();

    if (format == RBOInternalFormat::Depth24 || format == RBOInternalFormat::Depth32f)
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                GL_RENDERBUFFER, this->depthBuffer.getID());
    else if (format == RBOInternalFormat::Stencil)
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                GL_RENDERBUFFER, this->depthBuffer.getID());
    else if (format == RBOInternalFormat::DepthStencil)
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_RENDERBUFFER, this->depthBuffer.getID());
    this->unbind();

    this->clearFlags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    this->hasRenderBuffer = true;
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
      glDrawBuffers(totalAttachments.size(), totalAttachments.data());
    this->unbind();
  }

  void
  FrameBuffer::blitzToOther(FrameBuffer &target, const FBOTargetParam &type)
  {
    switch (type)
    {
      case FBOTargetParam::Depth:
      {
        auto otherSize = target.getSize();
        glBlitNamedFramebuffer(this->bufferID, target.getID(), 0, 0,
                               this->width, this->height, 0, 0,
                               (uint) otherSize.x, (uint) otherSize.y,
                               GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        break;
      }

      case FBOTargetParam::Stencil:
      {
        auto otherSize = target.getSize();
        glBlitNamedFramebuffer(this->bufferID, target.getID(), 0, 0,
                               this->width, this->height, 0, 0,
                               (uint) otherSize.x, (uint) otherSize.y,
                               GL_STENCIL_BUFFER_BIT, GL_NEAREST);
        break;
      }

      case FBOTargetParam::DepthStencil:
      {
        auto otherSize = target.getSize();
        glBlitNamedFramebuffer(this->bufferID, target.getID(), 0, 0,
                               this->width, this->height, 0, 0,
                               (uint) otherSize.x, (uint) otherSize.y,
                               GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                               GL_NEAREST);
        break;
      }

      default:
      {
        // Default to colour for any other attachment.
        auto otherSize = target.getSize();
        glBlitNamedFramebuffer(this->bufferID, target.getID(), 0, 0,
                               this->width, this->height, 0, 0,
                               (uint) otherSize.x, (uint) otherSize.y,
                               GL_COLOR_BUFFER_BIT, GL_LINEAR);
        break;
      }
    }
  }

  int
  FrameBuffer::readPixel(const FBOTargetParam &target, const glm::vec2 &mousePos)
  {
    this->bind();
    glReadBuffer(static_cast<GLenum>(target));
    float data;
    glReadPixels(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y), 1, 1, GL_RED, GL_FLOAT, &data);
    return static_cast<int>(glm::round(data));
  }

  // Resize the framebuffer.
  void
  FrameBuffer::resize(uint width, uint height)
  {
    this->width = width;
    this->height = height;

    // Update the attached textures.
    for (auto& attachments : this->textureAttachments)
    {
      auto& ownedAttachment = attachments.second;

      switch(ownedAttachment.type)
      {
        case FBOTextureParam::CubeMapPX:
        {
          break;
        }
        case FBOTextureParam::CubeMapNX:
        {
            break;
        }
        case FBOTextureParam::CubeMapPY:
        {
            break;
        }
        case FBOTextureParam::CubeMapNY:
        {
            break;
        }
        case FBOTextureParam::CubeMapPZ:
        {
            break;
        }
        case FBOTextureParam::CubeMapNZ:
        {
            break;
        }
        default:
        {
          glBindTexture(static_cast<GLenum>(ownedAttachment.type), ownedAttachment.attachmentID);
          glTexImage2D(static_cast<GLenum>(ownedAttachment.type), 0, 
                       static_cast<GLenum>(ownedAttachment.internal), this->width, this->height, 
                       0, static_cast<GLenum>(ownedAttachment.format),
                       static_cast<GLenum>(ownedAttachment.dataType), nullptr);
          glBindTexture(static_cast<GLenum>(ownedAttachment.type), 0);
        }
      }
    }

    // Update the attached render buffer.
    this->depthBuffer.reset(this->width, this->height);
  }

  void
  FrameBuffer::setViewport()
  {
    glViewport(0, 0, this->width, this->height);
  }

  void
  FrameBuffer::setClearColour(const glm::vec4 &clearColour)
  {
    this->clearColour = clearColour;
  }

  void
  FrameBuffer::bindTextureID(const FBOTargetParam &attachment)
  {
    auto& ownedAttachment = this->textureAttachments.at(attachment);
    glBindTexture(static_cast<GLenum>(ownedAttachment.type), ownedAttachment.attachmentID);
  }

  void
  FrameBuffer::bindTextureID(const FBOTargetParam &attachment, uint bindPoint)
  {
    auto& ownedAttachment = this->textureAttachments.at(attachment);
    glActiveTexture(GL_TEXTURE0 + bindPoint);
    glBindTexture(static_cast<GLenum>(ownedAttachment.type), ownedAttachment.attachmentID);
  }

  void 
  FrameBuffer::bindTextureIDAsImage(const FBOTargetParam& attachment, uint bindPoint,
                                    uint miplevel, bool isLayered, uint layer,
                                    ImageAccessPolicy policy)
  {
    auto& ownedAttachment = this->textureAttachments.at(attachment);
    glBindImageTexture(bindPoint, ownedAttachment.attachmentID, miplevel,
                       static_cast<GLenum>(isLayered), layer,
                       static_cast<GLenum>(policy),
                       static_cast<GLenum>(ownedAttachment.internal));
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
      Logs::log("The framebuffer is not valid for a drawcall.");
      this->unbind();
      return false;
    }
    else
    {
      this->unbind();
      return true;
    }
  }
}
