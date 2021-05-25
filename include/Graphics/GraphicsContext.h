#pragma once

// Macro include file.
#include "SciRenderPCH.h"

namespace SciRenderer
{
  // A wrapper class for the OpenGL (loaded through GLAD) graphics contexts.
  class GraphicsContext
  {
  public:
    GraphicsContext(GLFWwindow* genWindow);
    ~GraphicsContext();

    void init();

    void swapBuffers();

    std::string getContextInfo() { return this->contextInfo; }
  private:
    // The window with the context. Since we ask GLFW to generate a context for
    // us we need this window stored.
    GLFWwindow* glfwWindowRef;

    // A string of info surrounding the graphics context.
    std::string contextInfo;
  };
}
