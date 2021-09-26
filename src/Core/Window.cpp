#include "Core/Window.h"

// Project includes.
#include "Core/Events.h"

// GLFW includes.
#include <GLFW/glfw3.h>

namespace Strontium
{
  uint Window::windowInstances = 0;

  Window::Window(const std::string &name, const uint &width,
                 const uint &height, const bool &debug, const bool &setVSync)
    : initialized(false)
    , isDebug(debug)
    , hasVSync(setVSync)
  {
    this->properties.width = width;
    this->properties.height = height;
    this->properties.name = name;

    std::cout << "Generating a window with the name: " << name << ", and size: "
              << width << "x" << height << "." << std::endl;

    this->init();
  }

  Shared<Window> Window::getNewInstance(const std::string &name, const uint &width,
                                        const uint &height, const bool &debug,
                                        const bool &setVSync)
  {
    return createShared<Window>(name, width, height, debug, setVSync);
  }

  Window::~Window()
  {
    this->shutDown();
  }

  // Initializing the window.
  void
  Window::init()
  {
    // Set the error callback first, so we actually know whats going on if
    // something happens.
    glfwSetErrorCallback(
    [](int error, const char* description)
    {
      fprintf(stderr, "Error: %s\n", description);
    });

    // Initialize GLFW if there are no windows.
    if (Window::windowInstances == 0)
    {
      std::cout << "Initializing GLFW" << std::endl;

      if (!glfwInit())
      {
        std::cout << "Error initializing GLFW, aborting." << std::endl;
        exit(EXIT_FAILURE);
      }
    }

    // Create the actual window. If we want a debug window, set it to that.
    if (this->isDebug)
      glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    this->glfwWindowRef = glfwCreateWindow(this->properties.width, this->properties.height,
                                           this->properties.name.c_str(), nullptr,
                                           nullptr);
    if (!this->glfwWindowRef)
    {
      std::cout << "Error creating the window. Aborting." << std::endl;
      exit(EXIT_FAILURE);
    }
    Window::windowInstances++;

    // Generate a graphics context.
    this->glContext = new GraphicsContext(this->glfwWindowRef);
    this->glContext->init();

    // Set a custom window pointer so we can get member data in/out of callbacks.
    glfwSetWindowUserPointer(this->glfwWindowRef, &this->properties);

    // Enable / disable VSync.
    this->setVSync(hasVSync);

    this->initialized = true;

    // Set the callbacks so events can get pushed.
    // Key callback.
    glfwSetKeyCallback(this->glfwWindowRef,
    [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
      // Fetch the dispatcher.
      EventDispatcher* appEvents = EventDispatcher::getInstance();

      // Dispatch the event.
      switch (action)
      {
        case GLFW_PRESS:
          appEvents->queueEvent(new KeyPressedEvent(key, 0));
          break;
        case GLFW_RELEASE:
          appEvents->queueEvent(new KeyReleasedEvent(key));
          break;
        case GLFW_REPEAT:
          appEvents->queueEvent(new KeyPressedEvent(key, 1));
          break;
      }
    });

    // Character callback.
    glfwSetCharCallback(this->glfwWindowRef,
    [](GLFWwindow* window, unsigned int keycode)
    {
      // Fetch the dispatcher.
      EventDispatcher* appEvents = EventDispatcher::getInstance();

      // Dispatch the event.
      appEvents->queueEvent(new KeyTypedEvent(keycode));
    });

    // Mouse click callback.
    glfwSetMouseButtonCallback(this->glfwWindowRef,
    [](GLFWwindow* window, int button, int action, int mods)
    {
      // Fetch the dispatcher.
      EventDispatcher* appEvents = EventDispatcher::getInstance();

      // Dispatch the event.
      switch (action)
      {
        case GLFW_PRESS:
          appEvents->queueEvent(new MouseClickEvent(button));
          break;
        case GLFW_RELEASE:
          appEvents->queueEvent(new MouseReleasedEvent(button));
          break;
      }
    });

    // Mouse scroll callback.
    glfwSetScrollCallback(this->glfwWindowRef,
    [](GLFWwindow* window, double xOffset, double yOffset)
    {
      // Fetch the dispatcher.
      EventDispatcher* appEvents = EventDispatcher::getInstance();

      // Dispatch the event.
      appEvents->queueEvent(new MouseScrolledEvent((float) xOffset,
                                                   (float) yOffset));
    });

    // Mouse position callback.
    glfwSetCursorPosCallback(this->glfwWindowRef,
    [](GLFWwindow* window, double xPos, double yPos)
    {
      // Fetch the window data.
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

      // Set the window cursor position.
      data.cursorX = (float) xPos;
      data.cursorY = (float) yPos;
    });

    // Window resize callback.
    glfwSetWindowSizeCallback(this->glfwWindowRef,
    [](GLFWwindow* window, int width, int height)
    {
      // Fetch the dispatcher.
      EventDispatcher* appEvents = EventDispatcher::getInstance();

      // Dispatch the event.
      appEvents->queueEvent(new WindowResizeEvent((uint) width, (uint) height));

      // Fetch the window data.
      WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

      // Set the width and height of the window.
      data.width = width;
      data.height = height;
    });

    // Window closing callback.
    glfwSetWindowCloseCallback(this->glfwWindowRef,
    [](GLFWwindow* window)
    {
      // Fetch the dispatcher.
      EventDispatcher* appEvents = EventDispatcher::getInstance();

      // Dispatch the event.
      appEvents->queueEvent(new WindowCloseEvent());
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    });
  }

  // Shutting down the window.
  void
  Window::shutDown()
  {
    // Shutdown the window (also removes the graphics context since we load GLAD
    // with GLFW).
    glfwDestroyWindow(this->glfwWindowRef);
    delete glContext;
    Window::windowInstances --;

    // If there are no more windows, we terminate GLFW as well.
    if (Window::windowInstances == 0)
    {
      std::cout << "Shutting down GLFW" << std::endl;
      glfwTerminate();
    }
  }

  // Update the window.
  void
  Window::onUpdate()
  {
    glfwPollEvents();
		this->glContext->swapBuffers();
  }

  // Set the window to have VSync or not.
  void
  Window::setVSync(const bool &active)
  {
    if (active)
    {
      glfwSwapInterval(1);
    }
    else
    {
      glfwSwapInterval(0);
    }
  }

  glm::vec2
  Window::getCursorPos()
  {
    double mouseX, mouseY;
    glfwGetCursorPos(this->glfwWindowRef, &mouseX, &mouseY);

    return glm::vec2((float) mouseX, (float) mouseY);
  }

  void
  Window::setCursorCapture(const bool &active)
  {
    if (active)
      glfwSetInputMode(this->glfwWindowRef, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
      glfwSetInputMode(this->glfwWindowRef, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  float
  Window::getTime()
  {
    return glfwGetTime();
  }

  // Manually poll input.
  bool
  Window::isMouseClicked(const int &button)
  {
    return glfwGetMouseButton(this->glfwWindowRef, button) == GLFW_PRESS;
  }

  bool
  Window::isKeyPressed(const int &key)
  {
    int state = glfwGetKey(this->glfwWindowRef, key);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
  }
}
