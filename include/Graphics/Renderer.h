// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"
#include "Graphics/Meshes.h"
#include "Graphics/Camera.h"
#include "Graphics/FrameBuffer.h"

namespace SciRenderer
{
  // Singleton 3D rendering class.
  class Renderer3D
  {
  public:
    // Destructor.
    ~Renderer3D();

    // Get the renderer instance.
    static Renderer3D* getInstance();

    // Init the renderer object for drawing.
    void init(const std::string &vertPath, const std::string &fragPath);

    //--------------------------------------------------------------------------
    // Forward rendering setup.
    //--------------------------------------------------------------------------
    // Draw the data given.
    void draw(VertexArray* data, Shader* program);
    void draw(Mesh* data, Shader* program, Camera* camera);

    // Draws a full screen quad with a shader program.
    void drawFSQ(Shader* program);

    //--------------------------------------------------------------------------
    // Deferred rendering setup. WIP.
    //--------------------------------------------------------------------------
    // Begin the next frame.
    void begin();

    // Submit data to the render queue. TODO: Need to make this a material
    // instead of a shader.
    void submit(std::pair<Mesh*, Shader*> data);

    // End the next frame. Empty the render queue and draw everything to the
    // provided framebuffer.
    void end(FrameBuffer* drawBuffer);

  private:
    // Renderer instance.
    static Renderer3D* instance;

    std::queue<std::pair<Mesh*, Shader*>> renderQueue;

    // Shader to draw a texture to a screen.
    Shader* viewportProgram;

    // Viewport VAO and buffer ID.
    GLuint viewportVAOID;
    GLuint viewportVBOID;

    // Quad for the screen viewport.
    GLfloat viewport[24] =
    {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
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
