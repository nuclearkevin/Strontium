#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/GraphicsContext.h"

// Forward declare the window.
struct GLFWwindow;

namespace Strontium
{
  // A wrapper class for the GLFW window functionality.
  class Window
  {
  public:
    Window(const std::string &name, const uint &width, const uint &height,
           const bool &debug, const bool &setVSync);

    ~Window();

    // Ask for a new window.
    static Shared<Window> getNewInstance(const std::string &name = "Editor Viewport",
                                         const uint &width = 1920,
                                         const uint &height = 1080,
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

    float getTime();

    void setIcon(uint width, uint height, unsigned char* pixels);

    // Get the GLFW window pointer.
    GLFWwindow* getWindowPtr() { return this->glfwWindowRef; }
    // Get the window size.
    glm::ivec2 getSize() { return glm::ivec2(this->properties.width, this->properties.height); }
    // Get the cursor position.
    glm::vec2 getCursorPos();
    // Get the graphics context info.
    std::string getContextInfo() { return this->glContext->getContextInfo(); }

    // Check to see if a button / key is pressed.
    bool isMouseClicked(const int &button);
    bool isKeyPressed(const int &key);

    // A counter of window instances.
    static uint windowInstances;
  protected:
    bool initialized, isDebug, hasVSync;

    GLFWwindow* glfwWindowRef;
    GraphicsContext* glContext;

    struct WindowData
    {
      uint width;
      uint height;
      float cursorX;
      float cursorY;
      std::string name;
    };

    WindowData properties;
  };
}
