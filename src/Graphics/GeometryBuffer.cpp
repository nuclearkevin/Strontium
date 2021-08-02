#include "Graphics/GeometryBuffer.h"

namespace Strontium
{
  GeometryBuffer::GeometryBuffer()
    : type(RuntimeType::Editor)
  {
    this->geoBuffer = FrameBuffer(0, 0);
    this->geoBuffer.setClearColour(glm::vec4(0.0f));

    auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
    auto dSpec = FBOCommands::getDefaultDepthSpec();

    // The position texture.
    this->geoBuffer.attachTexture2D(cSpec);
    // The normal texture.
    cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1);
    this->geoBuffer.attachTexture2D(cSpec);
    // The albedo texture.
    cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour2);
    this->geoBuffer.attachTexture2D(cSpec);
    // The lighting materials texture.
    cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour3);
    this->geoBuffer.attachTexture2D(cSpec);
    // The ID texture with a mask for the current selected entity.
    cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour4);
    cSpec.sWrap = TextureWrapParams::ClampEdges;
    cSpec.tWrap = TextureWrapParams::ClampEdges;
    this->geoBuffer.attachTexture2D(cSpec);
    this->geoBuffer.setDrawBuffers();

    this->geoBuffer.attachTexture2D(dSpec);
    this->geoBuffer.bindTextureID(FBOTargetParam::Depth, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
  }

  GeometryBuffer::GeometryBuffer(const RuntimeType &type, GLuint width, GLuint height)
    : type(type)
  {
    this->geoBuffer = FrameBuffer(width, height);

    auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
    auto dSpec = FBOCommands::getDefaultDepthSpec();
    switch (type)
    {
      case RuntimeType::Editor:
      {
        // The position texture.
        this->geoBuffer.attachTexture2D(cSpec);
        // The normal texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1);
        this->geoBuffer.attachTexture2D(cSpec);
        // The albedo texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour2);
        this->geoBuffer.attachTexture2D(cSpec);
        // The lighting materials texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour3);
        this->geoBuffer.attachTexture2D(cSpec);
        // The ID texture with a mask for the current selected entity.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour4);
        cSpec.sWrap = TextureWrapParams::ClampEdges;
        cSpec.tWrap = TextureWrapParams::ClampEdges;
        this->geoBuffer.attachTexture2D(cSpec);
        this->geoBuffer.setDrawBuffers();
        break;
      }
      case RuntimeType::Runtime:
      {
        // The position texture.
        this->geoBuffer.attachTexture2D(cSpec);
        // The normal texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1);
        this->geoBuffer.attachTexture2D(cSpec);
        // The albedo texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour2);
        this->geoBuffer.attachTexture2D(cSpec);
        // The lighting materials texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour3);
        this->geoBuffer.attachTexture2D(cSpec);
        this->geoBuffer.setDrawBuffers();
        break;
      }
    }
    this->geoBuffer.attachTexture2D(dSpec);
    this->geoBuffer.bindTextureID(FBOTargetParam::Depth, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
  }

  void
  GeometryBuffer::beginGeoPass()
  {
    this->geoBuffer.clear();
    this->geoBuffer.bind();
    this->geoBuffer.setViewport();
  }

  void
  GeometryBuffer::endGeoPass()
  {
    this->geoBuffer.unbind();
  }

  void
  GeometryBuffer::swapType(const RuntimeType &type)
  {
    auto size = this->geoBuffer.getSize();
    this->type = type;
    this->geoBuffer = FrameBuffer(size.x, size.y);

    auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
    auto dSpec = FBOCommands::getDefaultDepthSpec();
    switch (type)
    {
      case RuntimeType::Editor:
      {
        // The position texture.
        this->geoBuffer.attachTexture2D(cSpec);
        // The normal texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1);
        this->geoBuffer.attachTexture2D(cSpec);
        // The albedo texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour2);
        this->geoBuffer.attachTexture2D(cSpec);
        // The lighting materials texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour3);
        this->geoBuffer.attachTexture2D(cSpec);
        // The ID texture with a mask for the current selected entity.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour4);
        cSpec.sWrap = TextureWrapParams::ClampEdges;
        cSpec.tWrap = TextureWrapParams::ClampEdges;
        this->geoBuffer.attachTexture2D(cSpec);
        this->geoBuffer.setDrawBuffers();
        break;
      }
      case RuntimeType::Runtime:
      {
        // The position texture.
        this->geoBuffer.attachTexture2D(cSpec);
        // The normal texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1);
        this->geoBuffer.attachTexture2D(cSpec);
        // The albedo texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour2);
        this->geoBuffer.attachTexture2D(cSpec);
        // The lighting materials texture.
        cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour3);
        this->geoBuffer.attachTexture2D(cSpec);
        this->geoBuffer.setDrawBuffers();
        break;
      }
    }
    this->geoBuffer.attachTexture2D(dSpec);
    this->geoBuffer.bindTextureID(FBOTargetParam::Depth, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
  }

  void
  GeometryBuffer::resize(GLuint width, GLuint height)
  {
    this->geoBuffer.resize(width, height);
  }

  void
  GeometryBuffer::blitzToOther(FrameBuffer &target, const FBOTargetParam &type)
  {
    this->geoBuffer.blitzToOther(target, type);
  }

  void
  GeometryBuffer::bindAttachment(const FBOTargetParam &attachment, GLuint bindPoint)
  {
    this->geoBuffer.bindTextureID(attachment, bindPoint);
  }
}
