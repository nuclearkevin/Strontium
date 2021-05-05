// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"
#include "Graphics/Meshes.h"
#include "Graphics/Camera.h"

namespace SciRenderer
{
  // Singleton rendering class.
  class Renderer
  {
  public:
    // Renderer instance.
    static Renderer* instance;

    // Destructor.
    ~Renderer();

    // Get the renderer instance.
    static Renderer* getInstance();

    // Init the renderer object for drawing.
    void init(const char* vertPath, const char* fragPath);

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
}
