#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Graphics/GraphicsContext.h"

namespace SciRenderer
{
  // A wrapper class for the GLFW window functionality.
  class Window
  {
  public:
    ~Window();

    // Ask for a new window.
    static Window* getNewInstance(const std::string &name = "Editor Viewport",
                                  const GLuint &width = 1600,
                                  const GLuint &height = 900,
                                  const bool &debug = false,
                                  const bool &setVSync = true);

    // Initialize/shutdown the window. Deals with the graphics context.
    void init();

    // Kills the window and the graphics context associated with it.
    void shutDown();

    // Update the window.
    void onUpdate();

    // Enable or disable VSync.
    void setVSync(const bool &active);

    // Set the cursor to be captured.
    void setCursorCapture(const bool &active);

    // Get the GLFW window pointer.
    inline GLFWwindow* getWindowPtr() { return this->glfwWindowRef; }
    // Get the window size.
    inline glm::ivec2 getSize() { return glm::ivec2(this->properties.width, this->properties.height); }
    // Get the cursor position.
    glm::vec2 getCursorPos();
    // Get the graphics context info.
    inline std::string getContextInfo() { return this->glContext->getContextInfo(); }

    // Check to see if a button / key is pressed.
    bool isMouseClicked(const int &button);
    bool isKeyPressed(const int &key);

    // A counter of window instances.
    static GLuint windowInstances;
  protected:
    Window(const std::string &name, const GLuint &width, const GLuint &height,
           const bool &debug, const bool &setVSync);

    bool initialized, isDebug, hasVSync;

    GLFWwindow* glfwWindowRef;
    GraphicsContext* glContext;

    struct WindowData
    {
      GLuint width;
      GLuint height;
      GLfloat cursorX;
      GLfloat cursorY;
      std::string name;
    };

    WindowData properties;
  };
}
