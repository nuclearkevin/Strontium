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
    Colour0 = GL_COLOR_ATTACHMENT0, Colour1 = GL_COLOR_ATTACHMENT1,
    Colour2 = GL_COLOR_ATTACHMENT2, Colour3 = GL_COLOR_ATTACHMENT3,
    Colour4 = GL_COLOR_ATTACHMENT4, Colour5 = GL_COLOR_ATTACHMENT5,
    Depth = GL_DEPTH_ATTACHMENT, Stencil = GL_STENCIL_ATTACHMENT,
    DepthStencil = GL_DEPTH_STENCIL
  };

  enum class FBOTex2DParam
  {
    Texture2D = GL_TEXTURE_2D,
    CubeMapPX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    CubeMapNX = GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1,
    CubeMapPY = GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2,
    CubeMapNY = GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3,
    CubeMapPZ = GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4,
    CubeMapNZ = GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5
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
    FrameBuffer(GLuint width, GLuint height);
    ~FrameBuffer();

    // Bind/unbind the FBO or its attachments.
    void bind();
    void unbind();
    void bindTextureID(const FBOTargetParam &attachment);
    void bindTextureID(const FBOTargetParam &attachment, GLuint bindPoint);

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
    GLint readPixel(const FBOTargetParam &target, const glm::vec2 &mousePos);

    // Update FBO properties.
    void resize(GLuint width, GLuint height);
    void setClearColour(const glm::vec4 &clearColour);

    // Update the framebuffer state.
    void clear();
    void setViewport();
    void setDrawBuffers();

    bool isValid();
    glm::vec2 getSize() { return glm::vec2(this->width, this->height); }
    GLuint getAttachID(const FBOTargetParam &attachment) { return this->textureAttachments.at(attachment).second->getID(); }
    GLuint getRenderBufferID() { return this->depthBuffer != nullptr ? this->depthBuffer->getID() : 0; };
    GLuint getID() { return this->bufferID; }
  protected:
    GLuint bufferID;

    std::unordered_map<FBOTargetParam, std::pair<FBOSpecification, Shared<Texture2D>>> textureAttachments;
    Shared<RenderBuffer> depthBuffer;

    GLuint width, height;

    bool hasRenderBuffer;

    GLbitfield clearFlags;

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
