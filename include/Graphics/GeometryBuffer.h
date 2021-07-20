#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/FrameBuffer.h"

namespace SciRenderer
{
  // A switch for differentiating between editor and runtime graphics modes.
  enum class RuntimeType
  {
    Editor, Runtime
  };

  class GeometryBuffer
  {
  public:
    GeometryBuffer(const RuntimeType &type, GLuint width, GLuint height);
    GeometryBuffer();
    ~GeometryBuffer() = default;

    void beginGeoPass();
    void endGeoPass();

    void swapType(const RuntimeType &type);

    void resize(GLuint width, GLuint height);
    void blitzToOther(FrameBuffer &target, const FBOTargetParam &type);

    void bindAttachment(const FBOTargetParam &attachment, GLuint bindPoint);
    GLuint getAttachmentID(const FBOTargetParam &attachment) { return this->geoBuffer.getAttachID(attachment); }
    glm::vec2 getSize() { return this->geoBuffer.getSize(); }
  private:
    RuntimeType type;

    FrameBuffer geoBuffer;
  };
}
