#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"

namespace SciRenderer
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

    // Bind and unbind the framebuffer. Have to unbind before rendering to the
    // default buffer.
    void bind();
    void unbind();

    // Methods for texture/buffer generation and attachment.
    void attachTexture2D(const FBOSpecification &spec, const bool &removeTex = true);
    void attachTexture2D(const FBOSpecification &spec, Shared<Texture2D> &tex,
                         const bool &removeTex = true);
    void attachCubeMapFace(const FBOSpecification &spec, Shared<CubeMap> &map,
                           const bool &removeTex = true, GLuint mip = 0);
    void attachRenderBuffer();
    void attachRenderBuffer(Shared<RenderBuffer> buffer);

    // Unattach a 2D texture. This won't delete the texture.
    Shared<Texture2D> unattachTexture2D(const FBOTargetParam &attachment);

    void setDrawBuffers();

    // Update the framebuffer size.
    void resize(GLuint width, GLuint height);

    // Set the viewport size.
    void setViewport();

    // Get the IDs of the attachments.
    GLuint getAttachID(const FBOTargetParam &attachment);
    void bindTextureID(const FBOTargetParam &attachment, GLuint bindPoint);
    GLuint getRenderBufferID();

    // Get the size of the framebuffer.
    glm::vec2 getSize() { return glm::vec2(this->width, this->height); }

    // Clear the buffer.
    void clear();

    // Check if the framebuffer is valid.
    bool isValid();

  protected:
    GLuint       bufferID;

    std::unordered_map<FBOTargetParam, std::pair<FBOSpecification, Shared<Texture2D>>> textureAttachments;
    Shared<RenderBuffer> depthBuffer;

    GLuint       width, height;

    bool         hasRenderBuffer;

    GLbitfield   clearFlags;

    glm::vec4    clearColour;
  };

  namespace FBOCommands
  {
    // Helper commands to get a colour and depth specification.
    FBOSpecification getDefaultColourSpec(const FBOTargetParam &attach);
    FBOSpecification getFloatColourSpec(const FBOTargetParam &attach);
    FBOSpecification getDefaultDepthSpec();
  };
}
