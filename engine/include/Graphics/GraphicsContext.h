#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

// Forward declare the window.
struct GLFWwindow;

namespace Strontium
{
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
