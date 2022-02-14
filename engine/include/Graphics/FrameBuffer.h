#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Textures.h"
#include "Graphics/RenderBuffer.h"

namespace Strontium
{
  // Framebuffer parameters.
  enum class FBOTargetParam
  {
    Colour0 = 0x8CE0,
    Colour1 = 0x8CE1,
    Colour2 = 0x8CE2,
    Colour3 = 0x8CE3,
    Colour4 = 0x8CE4,
    Colour5 = 0x8CE5,
    Depth = 0x8D00,
    Stencil = 0x8D20,
    DepthStencil = 0x84F9
  };

  enum class FBOTextureParam
  {
    Texture2D = 0x0DE1,
    CubeMapPX = 0x8515, // GL_TEXTURE_CUBE_MAP_POSITIVE_X
    CubeMapNX = 0x8516, // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
    CubeMapPY = 0x8517, // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
    CubeMapNY = 0x8518, // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    CubeMapPZ = 0x8519, // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
    CubeMapNZ = 0x851A // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
  };

  struct FBOAttachment
  {
    uint attachmentID;
    FBOTargetParam target;
    FBOTextureParam type;
    bool ownedByFBO;

    TextureInternalFormats internal;
    TextureFormats format;
    TextureDataType dataType;

    FBOAttachment(FBOTargetParam target, FBOTextureParam type,
                  TextureInternalFormats internal, TextureFormats format,
                  TextureDataType dataType)
      : attachmentID(0)
      , target(target)
      , type(type)
      , ownedByFBO(true)
      , internal(internal)
      , format(format)
      , dataType(dataType)
    { };

    FBOAttachment(uint id, FBOTargetParam target, FBOTextureParam type, 
                  TextureInternalFormats internal, TextureFormats format, 
                  TextureDataType dataType)
      : attachmentID(0)
      , target(target)
      , type(FBOTextureParam::Texture2D)
      , ownedByFBO(false)
      , internal(internal)
      , format(format)
      , dataType(dataType)
    { };
  };

  // Frame buffer class.
  class FrameBuffer
  {
  public:
    // A constructor to generate a framebuffer at a particular location or any
    // location. Default constructor sets the width and height to 0.
    FrameBuffer();
    FrameBuffer(uint width, uint height);
    ~FrameBuffer();

    // Delete the copy constructor and the assignment operator. Prevents
    // issues related to the underlying API.
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    // Bind/unbind the FBO or its attachments.
    void bind();
    void unbind();
    void bindTextureID(const FBOTargetParam &attachment);
    void bindTextureID(const FBOTargetParam &attachment, uint bindPoint);

    void bindTextureIDAsImage(const FBOTargetParam& attachment, uint bindPoint, 
                              uint miplevel, bool isLayered,  uint layer, 
                              ImageAccessPolicy policy);

    // Methods for texture/buffer generation and attachment.
    void attach(const Texture2DParams &params, const FBOAttachment &attachment);
    void attachRenderBuffer(RBOInternalFormat format = RBOInternalFormat::Depth32f);

    // Misc functions.
    void blitzToOther(FrameBuffer &target, const FBOTargetParam &type);
    int readPixel(const FBOTargetParam &target, const glm::vec2 &mousePos);

    // Update FBO properties.
    void resize(uint width, uint height);
    void setClearColour(const glm::vec4 &clearColour);

    // Update the framebuffer state.
    void clear();
    void setViewport();
    void setDrawBuffers();

    bool isValid();
    glm::vec2 getSize() { return glm::vec2(this->width, this->height); }
    uint getAttachID(const FBOTargetParam &attachment) { return this->textureAttachments.at(attachment).attachmentID; }
    uint getRenderBufferID() { return this->depthBuffer.getID(); };
    uint getID() { return this->bufferID; }
  protected:
    uint bufferID;

    std::map<FBOTargetParam, FBOAttachment> textureAttachments;
    RenderBuffer depthBuffer;

    uint width, height;

    bool hasRenderBuffer;

    uint clearFlags;

    glm::vec4 clearColour;
  };
}
