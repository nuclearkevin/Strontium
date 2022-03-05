// Include guard.
#pragma once

#define MAX_NUM_BLOOM_MIPS 7

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

    uint pointLightCount;
    std::array<PointLight, 1024> pointLightQueue;
    uint spotLightCount;
    std::array<SpotLight, 1024> spotLightQueue;

    VertexArray blankVAO;

    Texture2D lightingBuffer;
    Texture2D halfResBuffer1;

    GlobalRendererData()
      : blankVAO()
    { }
  };

  // Init the renderer for drawing.
  void init(const uint width, const uint height);
  void shutdown();

  GlobalRendererData& getStorage();
  RenderPassManager& getPassManager();

  // Draw the data given, forward rendering.
  void draw(VertexArray* data, Shader* program);

  // Generic begin and end for the renderer.
  void begin(uint width, uint height, const Camera &sceneCamera);
  void end(FrameBuffer& frontBuffer);

  // Deferred rendering setup.
  void submit(const PointLight &light, const glm::mat4 &model);
  void submit(const SpotLight &light, const glm::mat4 &model);
}
