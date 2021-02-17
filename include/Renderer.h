// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "VertexArray.h"
#include "Shaders.h"

namespace SciRenderer
{
  // Singleton rendering class.
  class Renderer
  {
  public:
    // Renderer instance.
    static Renderer* instance;

    // Destructor.
    ~Renderer() = default;

    // Get the renderer instance.
    static Renderer* getInstance();

    // Init the renderer object for drawing.
    void init(GLenum mode);

    // Draw the data given.
    void draw(VertexArray* data, Shader* program);
    void draw(Mesh* data, Shader* program);

    // Swap the buffers.
    void swap(GLFWwindow* window);

    // Clear the depth and colour buffers.
    void clear();

  private:
    // Constructor.
    Renderer() = default;
  };
}
