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

    uint numTransforms;
    std::vector<std::tuple<Model*, ModelMaterial*, glm::mat4, uint, bool>> staticRenderQueue;
    std::vector<std::tuple<Model*, Animator*, ModelMaterial*, glm::mat4, uint, bool>> dynamicRenderQueue;

    int primaryLightIndex;
    uint directionalLightCount;
    std::array<std::pair<DirectionalLight, std::bitset<2>>, 8> directionalLightQueue;
    uint pointLightCount;
    std::array<PointLight, 1024> pointLightQueue;
    uint spotLightCount;
    std::array<SpotLight, 1024> spotLightQueue;

    VertexArray blankVAO;

    Texture2D lightingBuffer;
    Texture2D halfResBuffer1;

    bool drawEdge;

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
  void submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
              float id = 0.0f, bool drawSelectionMask = false);
  void submit(Model* data, Animator* animation, ModelMaterial &materials,
              const glm::mat4 &model, float id = 0.0f,
              bool drawSelectionMask = false);
  void submit(const DirectionalLight &light, bool primaryLight, bool castShadows, const glm::mat4 &model);
  void submit(const PointLight &light, const glm::mat4 &model);
  void submit(const SpotLight &light, const glm::mat4 &model);
}
