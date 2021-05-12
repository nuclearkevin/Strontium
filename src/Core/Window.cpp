#include "Core/Window.h"

namespace SciRenderer
{
  GLuint Window::windowInstances = 0;

  Window* Window::getNewInstance(const std::string &name, const GLuint &width,
                                 const GLuint &height, const bool &debug,
                                 const bool &setVSync)
  {
    return new Window(name, width, height, debug, setVSync);
  }

  Window::Window(const std::string &name, const GLuint &width,
                 const GLuint &height, const bool &debug, const bool &setVSync)
    : initialized(false)
    , isDebug(debug)
    , hasVSync(setVSync)
    , width(width)
    , height(height)
    , name(name)
  {
    std::cout << "Generating a window with the name: " << name << ", and size: "
              << this->width << "x" << this->height << "." << std::endl;

    this->init();
  }

  Window::~Window()
  {
    this->shutDown();
  }

  // Initializing the window.
  void
  Window::init()
  {
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

    this->glfwWindowRef = glfwCreateWindow(this->width, this->height,
                                           this->name.c_str(), nullptr, nullptr);
    if (!this->glfwWindowRef)
    {
      std::cout << "Error creating the window. Aborting." << std::endl;
      exit(EXIT_FAILURE);
    }
    Window::windowInstances++;

    // Generate a graphics context.
    this->glContext = new GraphicsContext(this->glfwWindowRef);
    this->glContext->init();

    // Enable / disable VSync.
    this->setVSync(hasVSync);

    this->initialized = true;
  }

  // Shutting down the window.
  void
  Window::shutDown()
  {
    // Shutdown the window (also removes the graphics context since we load GLAD
    // with GLFW).
    glfwDestroyWindow(this->glfwWindowRef);
    Window::windowInstances --;

    // If there are no more windows, we terminate GLFW as well.
    if (Window::windowInstances == 0)
    {
      std::cout << "Shutting down GLFW" << std::endl;
      glfwTerminate();
    }
  }

  // Update the window.
  void Window::onUpdate()
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
}
