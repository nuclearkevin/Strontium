// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Math.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"

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

    Camera previousCamera;
    Frustum previousCamFrustum;

    uint time;

    UniformBuffer cameraBuffer;
    UniformBuffer temporalBuffer;

    VertexArray blankVAO;

    Unique<Texture2D> spatialBlueNoise;
    Unique<Texture2D> temporalBlueNoise;

    Texture2D lightingBuffer;
    Texture2D halfResBuffer1;
    Texture2D fullResBuffer1;

    GlobalRendererData()
      : blankVAO()
      , gamma(2.2f)
      , camFrustum()
      , previousCamFrustum()
      , time(0u)
      , cameraBuffer(3 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), BufferType::Dynamic)
      , temporalBuffer(4 * sizeof(glm::mat4) + sizeof(glm::vec4), BufferType::Dynamic)
    { }
  };

  GlobalRendererData& getStorage();
  RenderPassManager& getPassManager();

  // Init the renderer for drawing.
  void init(const uint width, const uint height);
  void shutdown();

  // Generic begin and end for the renderer.
  void begin(uint width, uint height, const Camera &sceneCamera, float dt);
  void end(FrameBuffer& frontBuffer);
}

namespace Strontium::Renderer2D
{
  struct GlobalRendererData
  {
    GlobalRendererData()
    { }
  };

  // Init the renderer for drawing.
  void init(const uint width, const uint height);
  void shutdown();
}

namespace Strontium::DebugRenderer
{
  struct GlobalRendererData
  {
    Camera sceneCam;

    UniformBuffer cameraBuffer;

    VertexArray blankVAO;

    bool visualizeAllColliders;

    GlobalRendererData()
      : sceneCam()
      , cameraBuffer(3 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4), BufferType::Dynamic)
      , blankVAO()
      , visualizeAllColliders(false)
    { }
  };

  GlobalRendererData& getStorage();
  RenderPassManager& getPassManager();

  // Init the renderer for drawing.
  void init(const uint width, const uint height);
  void shutdown();

  // Generic begin and end for the renderer.
  void begin(uint width, uint height, const Camera& sceneCamera);
  void end(FrameBuffer& frontBuffer);
}
