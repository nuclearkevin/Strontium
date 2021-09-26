#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/FrameBuffer.h"

namespace Strontium
{
  // A switch for differentiating between editor and runtime graphics modes.
  enum class RuntimeType
  {
    Editor, Runtime
  };

  class GeometryBuffer
  {
  public:
    GeometryBuffer(const RuntimeType &type, uint width, uint height);
    GeometryBuffer();
    ~GeometryBuffer() = default;

    void beginGeoPass();
    void endGeoPass();

    void swapType(const RuntimeType &type);

    void resize(uint width, uint height);
    void blitzToOther(FrameBuffer &target, const FBOTargetParam &type);

    void bindAttachment(const FBOTargetParam &attachment, uint bindPoint);
    uint getAttachmentID(const FBOTargetParam &attachment) { return this->geoBuffer.getAttachID(attachment); }
    glm::vec2 getSize() { return this->geoBuffer.getSize(); }
  private:
    RuntimeType type;

    FrameBuffer geoBuffer;
  };
}
