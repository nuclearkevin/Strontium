#include "Graphics/GeometryBuffer.h"

// OpenGL includes.
#include "glad/glad.h"

namespace Strontium
{
  GeometryBuffer::GeometryBuffer()
    : geoBuffer(1, 1)
  {
    this->geoBuffer.setClearColour(glm::vec4(0.0f));

    auto cSpec = Texture2D::getFloatColourParams();
    auto dSpec = Texture2D::getDefaultDepthParams();

    cSpec.sWrap = TextureWrapParams::ClampEdges;
    cSpec.tWrap = TextureWrapParams::ClampEdges;
    dSpec.sWrap = TextureWrapParams::ClampEdges;
    dSpec.tWrap = TextureWrapParams::ClampEdges;

    auto attachment = FBOAttachment(FBOTargetParam::Colour0, FBOTextureParam::Texture2D, 
                                    cSpec.internal, cSpec.format, cSpec.dataType);

    // The normal texture.
    this->geoBuffer.attach(cSpec, attachment);
    // The albedo texture.
    attachment.target = FBOTargetParam::Colour1;
    this->geoBuffer.attach(cSpec, attachment);
    // The lighting materials texture.
    attachment.target = FBOTargetParam::Colour2;
    this->geoBuffer.attach(cSpec, attachment);
    // The ID texture with a mask for the current selected entity.
    attachment.target = FBOTargetParam::Colour3;
    this->geoBuffer.attach(cSpec, attachment);
    this->geoBuffer.setDrawBuffers();

    attachment = FBOAttachment(FBOTargetParam::Depth, FBOTextureParam::Texture2D,
                               dSpec.internal, dSpec.format, dSpec.dataType);
    this->geoBuffer.attach(dSpec, attachment);
  }

  GeometryBuffer::GeometryBuffer(uint width, uint height)
    : geoBuffer(width, height)
  {
    auto cSpec = Texture2D::getFloatColourParams();
    auto dSpec = Texture2D::getDefaultDepthParams();

    cSpec.sWrap = TextureWrapParams::ClampEdges;
    cSpec.tWrap = TextureWrapParams::ClampEdges;
    dSpec.sWrap = TextureWrapParams::ClampEdges;
    dSpec.tWrap = TextureWrapParams::ClampEdges;

    auto attachment = FBOAttachment(FBOTargetParam::Colour0, FBOTextureParam::Texture2D,
                                    cSpec.internal, cSpec.format, cSpec.dataType);
    // The normal and tangent texture.
    this->geoBuffer.attach(cSpec, attachment);

    // The albedo texture.
    attachment.target = FBOTargetParam::Colour1;
    this->geoBuffer.attach(cSpec, attachment);

    // The lighting materials texture.
    attachment.target = FBOTargetParam::Colour2;
    this->geoBuffer.attach(cSpec, attachment);

    // The emission and anisotropic texture.
    attachment.target = FBOTargetParam::Colour3;
    this->geoBuffer.attach(cSpec, attachment);

    this->geoBuffer.setDrawBuffers();

    auto depthAttachment = FBOAttachment(FBOTargetParam::Depth, FBOTextureParam::Texture2D,
                                           dSpec.internal, dSpec.format, dSpec.dataType);
    this->geoBuffer.attach(dSpec, depthAttachment);
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
  GeometryBuffer::resize(uint width, uint height)
  {
    this->geoBuffer.resize(width, height);
  }

  void
  GeometryBuffer::blitzToOther(FrameBuffer &target, const FBOTargetParam &type)
  {
    this->geoBuffer.blitzToOther(target, type);
  }

  void
  GeometryBuffer::bindAttachment(const FBOTargetParam &attachment, uint bindPoint)
  {
    this->geoBuffer.bindTextureID(attachment, bindPoint);
  }
}
