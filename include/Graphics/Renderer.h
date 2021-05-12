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

    // Draw the data given.
    void draw(VertexArray* data, Shader* program);
    void draw(Mesh* data, Shader* program, Camera* camera);

    // Draw a populated framebuffer to a viewport (fullscreen quad).
    void drawToViewPort(FrameBuffer* drawBuffer);

    // Draws a full screen quad with a shader program.
    void drawFSQ(Shader* program);

    // Draw a texture to a viewport (fullscreen squad).
    void debugDrawTex(GLuint texID);

    // Swap the buffers.
    void swap(GLFWwindow* window);

  private:
    // Renderer instance.
    static Renderer3D* instance;

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

  enum class DepthFunctions
  {
    Less = GL_LESS,
    LEq = GL_LEQUAL
  };

  enum class RendererFunction
  {

  };

  namespace RendererCommands
  {
    void enable(const RendererFunction &toEnable);
    void depthFunction(const DepthFunctions &function);
    void setClearColour(const glm::vec4 &clearColour);
    void clear(const bool &clearColour = true, const bool &clearDepth = true,
               const bool &clearStencil = true);
    void setViewport(const glm::ivec2 topRight, const glm::ivec2 bottomLeft = glm::ivec2(0));
  };
}
