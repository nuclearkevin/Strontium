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

      DirectionalLight()
        : direction(glm::vec3(0.0f, -1.0f, 0.0f))
        , colour(glm::vec3(1.0f))
        , intensity(0.0f)
        , castShadows(false)
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
        , direction(glm::vec3(0.0f, -1.0f, 0.0f))
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

      bool isForward;

      VertexArray fsq;

      // The required buffers for processing.
      FrameBuffer geometryPass;
      FrameBuffer lightingPass;

      // The required shaders for processing.
      Shader* geometryShader;
      Shader* ambientShader;

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
    void end();

    // Deferred rendering setup.
    void submit(Model* data, ModelMaterial &materials, const glm::mat4 &model);
    void submit(DirectionalLight light, const glm::mat4 &model);
    void submit(PointLight light, const glm::mat4 &model);
    void submit(SpotLight light, const glm::mat4 &model);
  }

  // Depth functions. Addition to this as they are required.
  enum class DepthFunctions
  {
    Less = GL_LESS,
    LEq = GL_LEQUAL
  };

  // Render functions to glEnable. Adding to this as they are required.
  enum class RendererFunction
  {
    DepthTest = GL_DEPTH_TEST,
    CubeMapSeamless = GL_TEXTURE_CUBE_MAP_SEAMLESS
  };

  namespace RendererCommands
  {
    void enable(const RendererFunction &toEnable);
    void disable(const RendererFunction &toDisable);
    void depthFunction(const DepthFunctions &function);
    void setClearColour(const glm::vec4 &colour);
    void clear(const bool &clearColour = true, const bool &clearDepth = true,
               const bool &clearStencil = true);
    void setViewport(const glm::ivec2 topRight, const glm::ivec2 bottomLeft = glm::ivec2(0));
  };
}
