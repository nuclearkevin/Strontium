#define NUM_CASCADES 3

// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"
#include "Graphics/Meshes.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Camera.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/EnvironmentMap.h"
#include "Graphics/RendererCommands.h"

// STL includes.
#include <tuple>

namespace SciRenderer
{
  // The 3D renderer!
  namespace Renderer3D
  {
    struct DirectionalLight
    {
      glm::vec3 direction;
      glm::vec3 colour;
      GLfloat intensity;
      bool castShadows;
      bool primaryLight;

      DirectionalLight()
        : direction(glm::vec3(0.0f, 1.0f, 0.0f))
        , colour(glm::vec3(1.0f))
        , intensity(0.0f)
        , castShadows(false)
        , primaryLight(false)
      { }
    };

    struct PointLight
    {
      glm::vec3 position;
      glm::vec3 colour;
      GLfloat intensity;
      GLfloat radius;
      bool castShadows;

      PointLight()
        : position(glm::vec3(0.0f))
        , colour(glm::vec3(1.0f))
        , intensity(0.0f)
        , radius(0.0f)
        , castShadows(false)
      { }
    };

    struct SpotLight
    {
      glm::vec3 position;
      glm::vec3 direction;
      glm::vec3 colour;
      GLfloat intensity;
      GLfloat innerCutoff;
      GLfloat outerCutoff;
      GLfloat radius;
      bool castShadows;

      SpotLight()
        : position(glm::vec3(0.0f))
        , direction(glm::vec3(0.0f, 1.0f, 0.0f))
        , colour(glm::vec3(1.0f))
        , intensity(0.0f)
        , innerCutoff(std::cos(glm::radians(45.0f)))
        , outerCutoff(std::cos(glm::radians(90.0f)))
        , radius(0.0f)
        , castShadows(false)
      { }
    };

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

      // The required buffers for processing.
      FrameBuffer geometryPass;
      FrameBuffer shadowBuffer[NUM_CASCADES];
      FrameBuffer lightingPass;

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
      Shader* directionalShader;
      Shader* hdrPostShader;
      Shader* outlineShader;

      Unique<EnvironmentMap> currentEnvironment;
      Shared<Camera> sceneCam;

      std::vector<DirectionalLight> directionalQueue;
      std::vector<PointLight> pointQueue;
      std::vector<SpotLight> spotQueue;

      RendererStorage()
      {
        currentEnvironment = createUnique<EnvironmentMap>("./assets/models/cube.obj");
      }
    };

    // The renderer states (for serialization and other things of that nature).
    struct RendererState
    {
      // Settings for rendering.
      bool isForward;

      // Environment map settings.
      GLuint skyboxWidth;
      GLuint irradianceWidth;
      GLuint prefilterWidth;
      GLuint prefilterSamples;

      RendererState()
        : isForward(false)
        , skyboxWidth(512)
        , irradianceWidth(128)
        , prefilterWidth(512)
        , prefilterSamples(1024)
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

      RendererStats()
        : drawCalls(0)
        , numVertices(0)
        , numTriangles(0)
        , numDirLights(0)
        , numPointLights(0)
        , numSpotLights(0)
      { }
    };

    // Init the renderer for drawing.
    void init(const GLuint width, const GLuint height);
    void shutdown();

    RendererStorage* getStorage();
    RendererState* getState();
    RendererStats* getStats();

    // Generic begin and end for the renderer.
    void begin(GLuint width, GLuint height, Shared<Camera> sceneCam, bool isForward = false);
    void end(Shared<FrameBuffer> frontBuffer);

    // Deferred rendering setup.
    void submit(Model* data, ModelMaterial &materials, const glm::mat4 &model,
                GLfloat id = 0.0f, bool drawSelectionMask = false);
    void submit(DirectionalLight light);
    void submit(PointLight light, const glm::mat4 &model);
    void submit(SpotLight light, const glm::mat4 &model);
  }
}
