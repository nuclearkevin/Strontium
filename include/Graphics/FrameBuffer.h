#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"

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

  enum class FBOTex2DParam
  {
    Texture2D = 0x0DE1,
    CubeMapPX = 0x8515, // GL_TEXTURE_CUBE_MAP_POSITIVE_X
    CubeMapNX = 0x8516, // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
    CubeMapPY = 0x8517, // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
    CubeMapNY = 0x8518, // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    CubeMapPZ = 0x8519, // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
    CubeMapNZ = 0x851A // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
  };

  // Framebuffer specifications. Use this struct template to generate a
  // framebuffer colour texture.
  struct FBOSpecification
  {
    FBOTargetParam target;
    FBOTex2DParam type;

    TextureInternalFormats internal;
    TextureFormats         format;
    TextureDataType        dataType;

    TextureWrapParams      sWrap;
    TextureWrapParams      tWrap;
    TextureMinFilterParams minFilter;
    TextureMaxFilterParams maxFilter;

    FBOSpecification()
      : target(FBOTargetParam::Colour0)
      , type(FBOTex2DParam::Texture2D)
      , internal(TextureInternalFormats::RGB)
      , format(TextureFormats::RGB)
      , dataType(TextureDataType::Bytes)
      , sWrap(TextureWrapParams::Repeat)
      , tWrap(TextureWrapParams::Repeat)
      , minFilter(TextureMinFilterParams::Linear)
      , maxFilter(TextureMaxFilterParams::Linear)
    { };

    operator Texture2DParams() const
    {
      Texture2DParams outParams = Texture2DParams();
      outParams.sWrap = sWrap;
      outParams.tWrap = tWrap;
      outParams.minFilter = minFilter;
      outParams.maxFilter = maxFilter;
      outParams.internal = internal;
      outParams.format = format;
      outParams.dataType = dataType;
      return outParams;
    }
  };

  namespace Textures
  {
    const FBOTex2DParam cubemapFaces[6] =
    {
      FBOTex2DParam::CubeMapPX, FBOTex2DParam::CubeMapNX,
      FBOTex2DParam::CubeMapPY, FBOTex2DParam::CubeMapNY,
      FBOTex2DParam::CubeMapPZ, FBOTex2DParam::CubeMapNZ
    };
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

    // Bind/unbind the FBO or its attachments.
    void bind();
    void unbind();
    void bindTextureID(const FBOTargetParam &attachment);
    void bindTextureID(const FBOTargetParam &attachment, uint bindPoint);

    // Methods for texture/buffer generation and attachment.
    void attachTexture2D(const FBOSpecification &spec, const bool &removeTex = true);
    void attachTexture2D(const FBOSpecification &spec, Shared<Texture2D> &tex,
                         const bool &removeTex = true);
    void attachRenderBuffer(RBOInternalFormat format = RBOInternalFormat::Depth32f);

    // Detach/reattach FBO attachments. Doesn't delete the attachment from the
    // FBO's storage.
    void detach(const FBOTargetParam &attachment);
    void reattach(const FBOTargetParam &attachment);

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
    uint getAttachID(const FBOTargetParam &attachment) { return this->textureAttachments.at(attachment).second->getID(); }
    Shared<Texture2D> getAttachment(const FBOTargetParam &attachment) { return this->textureAttachments.at(attachment).second; }
    uint getRenderBufferID() { return this->depthBuffer != nullptr ? this->depthBuffer->getID() : 0; };
    uint getID() { return this->bufferID; }
  protected:
    uint bufferID;

    std::unordered_map<FBOTargetParam, std::pair<FBOSpecification, Shared<Texture2D>>> textureAttachments;
    Shared<RenderBuffer> depthBuffer;

    uint width, height;

    bool hasRenderBuffer;

    uint clearFlags;

    glm::vec4 clearColour;
  };

  namespace FBOCommands
  {
    // Helper commands to get a colour and depth specification.
    FBOSpecification getDefaultColourSpec(const FBOTargetParam &attach);
    FBOSpecification getFloatColourSpec(const FBOTargetParam &attach);
    FBOSpecification getDefaultDepthSpec();
  };
}
