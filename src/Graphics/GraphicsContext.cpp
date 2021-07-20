#include "Graphics/GraphicsContext.h"

namespace SciRenderer
{
  // Generic constructor / destructor pair.
  GraphicsContext::GraphicsContext(GLFWwindow* genWindow)
    : glfwWindowRef(genWindow)
  {
    if (this->glfwWindowRef == nullptr)
    {
      std::cout << "Window was nullptr, aborting." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  GraphicsContext::~GraphicsContext()
  { }

  void
  GraphicsContext::init()
  {
    glfwMakeContextCurrent(this->glfwWindowRef);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
      std::cout << "Failed to load a graphics context!" << std::endl;
      exit(EXIT_FAILURE);
    }
    auto vendor = glGetString(GL_VENDOR);
    auto device = glGetString(GL_RENDERER);
    auto version = glGetString(GL_VERSION);

    // Print info about the vendor to the console (for now).
    std::cout << "Graphics device vendor: " << vendor << std::endl;
    std::cout << "Graphics device: " << device << std::endl;
    std::cout << "Graphics context version: " << version << std::endl;

    this->contextInfo = std::string("Graphics device vendor: ") +
                        std::string(reinterpret_cast<const char*>(vendor)) +
                        std::string("\nGraphics device: ") +
                        std::string(reinterpret_cast<const char*>(device)) +
                        std::string("\nGraphics context version: ") +
                        std::string(reinterpret_cast<const char*>(version));

    // Check the version of the OpenGL context. Requires 4.4 or greater.
    GLuint major = (unsigned int) version[0] - 48;
    GLuint minor = (unsigned int) version[2] - 48;
    if (major < 4)
    {
      std::cout << "Unsupported OpenGL version, application requires OpenGL "
                << "core 4.4 or greater." << std::endl;
      exit(EXIT_FAILURE);
    }
    else if (major == 4 && minor < 4)
    {
      std::cout << "Unsupported OpenGL version, application requires OpenGL "
                << "core 4.4 or greater." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  void
  GraphicsContext::swapBuffers()
  {
    glfwSwapBuffers(this->glfwWindowRef);
  }
}
