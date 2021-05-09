#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/Textures.h"
#include "Graphics/Buffers.h"

// STL includes.
#include <unordered_map>
#include <utility>

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

  // Frame buffer class.
  class FrameBuffer
  {
  public:
    // A constructor to generate a framebuffer at a particular location or any
    // location.
    FrameBuffer(GLuint width, GLuint height);
    ~FrameBuffer();

    // Bind and unbind the framebuffer. Have to unbind before rendering to the
    // default buffer.
    void bind();
    void unbind();

    // Methods for texture/buffer generation and attachment.
    void attachTexture2D(const FBOSpecification &spec);
    void attachTexture2D(const FBOSpecification &spec, Texture2D* tex);
    void attachRenderBuffer();
    void attachRenderBuffer(RenderBuffer* buffer);
    void attachCubeMapFace();

    // Update the framebuffer size.
    void resize(GLuint width, GLuint height);

    // Get the size of the framebuffer.
    void setViewport();

    // Get the IDs of the attachments.
    GLuint getAttachID(const FBOTargetParam &attachment);
    GLuint getRenderBufferID();

    // Get the size of the framebuffer.
    void getSize(GLuint &outWidth, GLuint &outHeight);

    // Clear the buffer.
    void clear();

    // Check if the framebuffer is valid.
    bool isValid();

  protected:
    GLuint       bufferID;

    std::vector<FBOTargetParam> currentAttachments;
    std::unordered_map<FBOTargetParam, std::pair<FBOSpecification, Texture2D*>> textureAttachments;
    RenderBuffer* depthBuffer;

    GLuint       width, height;

    bool         hasRenderBuffer;

    GLbitfield   clearFlags;

    glm::vec4    clearColour;
  };

  namespace FrameBufferCommands
  {
    // Helper commands to get a colour and depth specification.
    FBOSpecification getDefaultColourSpec(const FBOTargetParam &attach);
    FBOSpecification getFloatColourSpec(const FBOTargetParam &attach);
    FBOSpecification getDefaultDepthSpec();
  };
}
