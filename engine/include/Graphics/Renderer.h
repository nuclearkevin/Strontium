// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Math.h"
#include "Graphics/RendererCommands.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/GeometryBuffer.h"

#include "Graphics/Meshes.h"
#include "Graphics/Model.h"
#include "Graphics/Animations.h"
#include "Graphics/Material.h"
#include "Graphics/ShadingPrimatives.h"

#include "Graphics/RenderPasses/RenderPassManager.h"

namespace Strontium::Renderer3D
{
  // The renderer storage.
  struct GlobalRendererData
  {
    Camera sceneCam;
    float gamma;
    Frustum camFrustum;

    VertexArray blankVAO;

    Texture2D lightingBuffer;
    Texture2D halfResBuffer1;

    GlobalRendererData()
      : blankVAO()
    { }
  };

  GlobalRendererData& getStorage();
  RenderPassManager& getPassManager();

  // Init the renderer for drawing.
  void init(const uint width, const uint height);
  void shutdown();

  // Generic begin and end for the renderer.
  void begin(uint width, uint height, const Camera &sceneCamera);
  void end(FrameBuffer& frontBuffer);
}
