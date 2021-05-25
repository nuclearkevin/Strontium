// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"
#include "Graphics/Meshes.h"
#include "Graphics/Camera.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/EnvironmentMap.h"

namespace SciRenderer
{
  struct RendererStorage
  {
    // Size of the buffers.
    GLuint width;
    GLuint height;

    // The required buffers for processing.
    FrameBuffer geometryPass;
    FrameBuffer lightingPass;

    // The required shaders for processing.
    Shader geometryShader;
    Shader lightingShader;

    // The renderer queue (stores the models and materials).
    std::vector<std::pair<Mesh*, Shader*>> renderQueue;

    // TODO: Add lighting queue with light volumes. Uniform lights are tricky...

    // Other components of a PBR scene.
    Camera* sceneCam;
    EnvironmentMap* currentEnvironment;
  };

  // Singleton 3D rendering class.
  class Renderer3D
  {
  public:
    // Destructor.
    ~Renderer3D();

    // Get the renderer instance.
    static Renderer3D* getInstance();

    // Init the renderer for drawing.
    void init(const GLuint width = 1600, const GLuint height = 900);

    //--------------------------------------------------------------------------
    // Forward rendering setup.
    //--------------------------------------------------------------------------
    // Draw the data given.
    void draw(Shared<VertexArray> data, Shared<Shader> program);
    void draw(Shared<Mesh> data, Shared<Shader> program, const glm::mat4 &model, Shared<Camera> camera);
    void draw(Shared<EnvironmentMap> environment, Shared<Camera> camera);

  private:
    // Renderer instance.
    static Renderer3D* instance;

    // Renderer storage.
    RendererStorage deferredStorage;

    // Shader to draw a texture to a screen.
    Shared<Shader> viewportProgram;
  };

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
    void depthFunction(const DepthFunctions &function);
    void setClearColour(const glm::vec4 &colour);
    void clear(const bool &clearColour = true, const bool &clearDepth = true,
               const bool &clearStencil = true);
    void setViewport(const glm::ivec2 topRight, const glm::ivec2 bottomLeft = glm::ivec2(0));
  };
}
