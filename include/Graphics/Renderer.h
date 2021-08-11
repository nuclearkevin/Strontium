#define NUM_CASCADES 4

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
#include "Graphics/Compute.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/GeometryBuffer.h"

#include "Graphics/EnvironmentMap.h"
#include "Graphics/Meshes.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Camera.h"
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
      GLuint width;
      GLuint height;

      // Various properties for rendering.
      bool isForward;
      bool drawEdge;

      // Geometric primatives for applying lights and effects.
      VertexArray fsq;

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

      // Items for the geometry pass.
      std::vector<std::tuple<Model*, ModelMaterial*, glm::mat4, GLuint, bool>> renderQueue;

      // Items for the shadow pass.
      std::vector<std::pair<Model*, glm::mat4>> shadowQueue;
      glm::mat4 cascades[NUM_CASCADES];
      GLfloat cascadeSplits[NUM_CASCADES];
      bool hasCascades;

      // The required shaders for processing.
      Shader* geometryShader;
      Shader* shadowShader;
      Shader* ambientShader;
      Shader* directionalShaderShadowed;
      Shader* directionalShader;
      Shader* pointShader;
      Shader* horBlur;
      Shader* verBlur;
      Shader* hdrPostShader;
      Shader* outlineShader;
      Shader* gridShader;

      Unique<EnvironmentMap> currentEnvironment;

      Shared<Camera> sceneCam;
      Frustum camFrustum;

      std::vector<DirectionalLight> directionalQueue;
      std::vector<PointLight> pointQueue;
      std::vector<SpotLight> spotQueue;

      RendererStorage()
        : camBuffer(2 * sizeof(glm::mat4) + sizeof(glm::vec3), BufferType::Dynamic)
        , transformBuffer(sizeof(glm::mat4), BufferType::Dynamic)
        , editorBuffer(sizeof(glm::vec4), BufferType::Dynamic)
        , ambientPassBuffer(sizeof(glm::vec4), BufferType::Dynamic)
        , directionalPassBuffer(2 * sizeof(glm::vec4) + sizeof(glm::vec2), BufferType::Dynamic)
        , pointPassBuffer(3 * sizeof(glm::vec4), BufferType::Dynamic)
        , cascadeShadowPassBuffer(sizeof(glm::mat4), BufferType::Dynamic)
        , cascadeShadowBuffer(NUM_CASCADES * sizeof(glm::mat4)
                              + NUM_CASCADES * sizeof(glm::vec4) * sizeof(GLfloat),
                              BufferType::Dynamic)
      {
        currentEnvironment = createUnique<EnvironmentMap>();
      }
    };

    // The renderer states (for serialization and other things of that nature).
    struct RendererState
    {
      // Current frame counter. Counts frames in 5 frame intervals.
      GLuint currentFrame;

      // Settings for rendering.
      bool isForward;
      bool frustumCull;

      // Environment map settings.
      GLuint skyboxWidth;
      GLuint irradianceWidth;
      GLuint prefilterWidth;
      GLuint prefilterSamples;

      // Cascaded shadow settings.
      GLfloat cascadeLambda;
      GLuint cascadeSize;
      GLfloat bleedReduction;

      // Some editor settings.
      bool drawGrid;

      RendererState()
        : currentFrame(0)
        , isForward(false)
        , frustumCull(false)
        , skyboxWidth(512)
        , irradianceWidth(128)
        , prefilterWidth(512)
        , prefilterSamples(1024)
        , cascadeLambda(0.5f)
        , cascadeSize(2048)
        , bleedReduction(0.2f)
        , drawGrid(true)
      { }
    };

    struct RendererStats
    {
      GLuint drawCalls;
      GLuint numVertices;
      GLuint numTriangles;
      GLuint numDirLights;
      GLuint numPointLights;
      GLuint numSpotLights;

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
    void init(const GLuint width, const GLuint height);
    void shutdown();

    RendererStorage* getStorage();
    RendererState* getState();
    RendererStats* getStats();

    // Draw the data given, forward rendering.
    void draw(VertexArray* data, Shader* program);
    void drawEnvironment();

    // Generic begin and end for the renderer.
    void begin(GLuint width, GLuint height, Shared<Camera> sceneCam, bool isForward = false);
    void end(Shared<FrameBuffer> frontBuffer);

    // Deferred rendering setup.
    void submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
                GLfloat id = 0.0f, bool drawSelectionMask = false);
    void submit(DirectionalLight light, const glm::mat4 &model);
    void submit(PointLight light, const glm::mat4 &model);
    void submit(SpotLight light, const glm::mat4 &model);
  }
}
