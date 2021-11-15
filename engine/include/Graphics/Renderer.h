// Include guard.
#pragma once

#define NUM_CASCADES 4
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

#include "Graphics/EnvironmentMap.h"
#include "Graphics/Meshes.h"
#include "Graphics/Model.h"
#include "Graphics/Animations.h"
#include "Graphics/Material.h"
#include "Graphics/ShadingPrimatives.h"

// STL includes.
#include <tuple>

namespace Strontium
{
  typedef int Renderer3DFlags;
  enum Renderer3DFlags_
  {
    Renderer3DFlags_None = 0,
    Renderer3DFlags_DrawMask = 1 << 0,
    Renderer3DFlags_DrawGrid = 1 << 1,
    Renderer3DFlags_DrawOutline = 1 << 2
  };

  // The 3D renderer!
  namespace Renderer3D
  {
    // The renderer storage.
    struct RendererStorage
    {
      // Size of the buffers.
      uint width;
      uint height;

      // Various properties for rendering.
      bool drawEdge;

      // Blank VAO for launching bufferless rendering.
      VertexArray blankVAO;

      // The required framebuffers.
      GeometryBuffer gBuffer;
      FrameBuffer shadowBuffer[NUM_CASCADES];
      FrameBuffer shadowEffectsBuffer;
      FrameBuffer lightingPass;

      // Uniform buffers.
      UniformBuffer camBuffer;
      UniformBuffer transformBuffer;
      UniformBuffer editorBuffer;
      UniformBuffer ambientPassBuffer;
      UniformBuffer directionalPassBuffer;
      UniformBuffer pointPassBuffer; // TEMP until tiled deferred is implemented.
      UniformBuffer cascadeShadowBuffer;
      UniformBuffer cascadeShadowPassBuffer;
      UniformBuffer postProcessSettings;

      // SSBO for bones.
      ShaderStorageBuffer boneBuffer;

      // Required objects for bloom.
      Texture2D downscaleBloomTex[MAX_NUM_BLOOM_MIPS];
      Texture2D bufferBloomTex[MAX_NUM_BLOOM_MIPS - 1];
      Texture2D upscaleBloomTex[MAX_NUM_BLOOM_MIPS];
      ShaderStorageBuffer bloomSettingsBuffer;

      // Required parameters for volumetric light shafts.
      Texture2D downsampleLightshaft;
      Texture2D halfResBuffer1;
      ShaderStorageBuffer lightShaftSettingsBuffer;

      // Items for the geometry pass.
      std::vector<std::tuple<Model*, ModelMaterial*, glm::mat4, uint, bool>> staticRenderQueue;
      std::vector<std::tuple<Model*, Animator*, ModelMaterial*, glm::mat4, uint, bool>> dynamicRenderQueue;

      // Items for the shadow pass.
      std::vector<std::pair<Model*, glm::mat4>> staticShadowQueue;
      std::vector<std::tuple<Model*, Animator*, glm::mat4>> dynamicShadowQueue;
      glm::mat4 cascades[NUM_CASCADES];
      float cascadeSplits[NUM_CASCADES];
      bool hasCascades;

      Unique<EnvironmentMap> currentEnvironment;

      Camera sceneCam;
      Frustum camFrustum;

      // Light queues.
      std::vector<DirectionalLight> directionalQueue;
      std::vector<PointLight> pointQueue;
      std::vector<SpotLight> spotQueue;

      RendererStorage()
        : blankVAO()
        , camBuffer(2 * sizeof(glm::mat4) + sizeof(glm::vec3), BufferType::Dynamic)
        , transformBuffer(sizeof(glm::mat4), BufferType::Dynamic)
        , editorBuffer(sizeof(glm::vec4), BufferType::Dynamic)
        , ambientPassBuffer(sizeof(glm::vec4), BufferType::Dynamic)
        , directionalPassBuffer(2 * sizeof(glm::vec4) + sizeof(glm::ivec4), BufferType::Dynamic)
        , pointPassBuffer(sizeof(PointLight), BufferType::Dynamic)
        , cascadeShadowPassBuffer(sizeof(glm::mat4), BufferType::Dynamic)
        , cascadeShadowBuffer(NUM_CASCADES * sizeof(glm::mat4)
                              + NUM_CASCADES * sizeof(glm::vec4) * sizeof(float),
                              BufferType::Dynamic)
        , postProcessSettings(2 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4)
                              + sizeof(glm::ivec4), BufferType::Dynamic)
        , boneBuffer(MAX_BONES_PER_MODEL * sizeof(glm::mat4), BufferType::Dynamic)
        , lightShaftSettingsBuffer(2 * sizeof(glm::vec4), BufferType::Dynamic)
        , bloomSettingsBuffer(sizeof(glm::vec4) + sizeof(float), BufferType::Dynamic)
      {
        currentEnvironment = createUnique<EnvironmentMap>();
      }
    };

    // The renderer states (for serialization and other things of that nature).
    struct RendererState
    {
      // Current frame counter. Counts frames in 5 frame intervals.
      uint currentFrame;

      // Settings for rendering.
      bool isForward;
      bool frustumCull;

      // Environment map settings.
      uint skyboxWidth;
      uint irradianceWidth;
      uint prefilterWidth;
      uint prefilterSamples;

      // Cascaded shadow settings.
      float cascadeLambda;
      uint cascadeSize;
      float bleedReduction;
      glm::ivec4 directionalSettings;

      // Volumetric light settings.
      bool enableSkyshafts;
      glm::vec4 mieScatIntensity;
      glm::vec4 mieAbsDensity;

      // HDR settings.
      float gamma;

      // Bloom settings.
      bool enableBloom;
      float bloomThreshold;
      float bloomKnee;
      float bloomIntensity;
      float bloomRadius;

      bool enableFXAA;

      // Post processing indicators for the post processing megashader.
      glm::ivec4 postProcessSettings;

      // Some editor settings.
      bool drawGrid;

      RendererState()
        : currentFrame(0)
        , isForward(false)
        , frustumCull(false)
        , skyboxWidth(512)
        , irradianceWidth(128)
        , prefilterWidth(512)
        , prefilterSamples(512)
        , cascadeLambda(0.5f)
        , cascadeSize(2048)
        , bleedReduction(0.2f)
        , directionalSettings(0)
        , enableSkyshafts(false)
        , mieScatIntensity(4.0f, 4.0f, 4.0f, 1.0f)
        , mieAbsDensity(4.4f, 4.4f, 4.4f, 1.0f)
        , gamma(2.2f)
        , enableBloom(true)
        , bloomThreshold(1.0f)
        , bloomKnee(1.0f)
        , bloomIntensity(1.0f)
        , bloomRadius(1.0f)
        , enableFXAA(false)
        , postProcessSettings(0) // Tone mapping operator (x), compositing bloom (y), FXAA (z). w is unused.
        , drawGrid(true)
      { }
    };

    struct RendererStats
    {
      uint drawCalls;
      uint numVertices;
      uint numTriangles;
      uint numDirLights;
      uint numPointLights;
      uint numSpotLights;

      float geoFrametime;
      float shadowFrametime;
      float lightFrametime;
      float postFramtime;

      RendererStats()
        : drawCalls(0)
        , numVertices(0)
        , numTriangles(0)
        , numDirLights(0)
        , numPointLights(0)
        , numSpotLights(0)
        , geoFrametime(0.0f)
        , shadowFrametime(0.0f)
        , lightFrametime(0.0f)
        , postFramtime(0.0f)
      { }
    };

    // Init the renderer for drawing.
    void init(const uint width, const uint height);
    void shutdown();

    RendererStorage* getStorage();
    RendererState* getState();
    RendererStats* getStats();

    // Draw the data given, forward rendering.
    void draw(VertexArray* data, Shader* program);
    void drawEnvironment();

    // Generic begin and end for the renderer.
    void begin(uint width, uint height, const Camera &sceneCamera);
    void end(Shared<FrameBuffer> frontBuffer);

    // Deferred rendering setup.
    void submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
                float id = 0.0f, bool drawSelectionMask = false);
    void submit(Model* data, Animator* animation, ModelMaterial &materials,
                const glm::mat4 &model, float id = 0.0f,
                bool drawSelectionMask = false);
    void submit(DirectionalLight light, const glm::mat4 &model);
    void submit(PointLight light, const glm::mat4 &model);
    void submit(SpotLight light, const glm::mat4 &model);
  }
}
