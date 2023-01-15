#include "Graphics/GraphicsContext.h"

// GLFW and OpenGL includes.
#include "glad/glad.h"
#include <GLFW/glfw3.h>

// Force dedicated graphics.
#ifdef SR_WINDOWS
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif // SR_WINDOWS

#ifdef SR_UNIX

#endif // SR_UNIX

namespace Strontium
{
  // Generic constructor / destructor pair.
  GraphicsContext::GraphicsContext(GLFWwindow* genWindow)
    : glfwWindowRef(genWindow)
    , contextInfo("")
    , versionMajor(0)
    , versionMinor(0)
    , stageTextureUnits(0)
    , stageUniformBuffers(0)
  {
    if (this->glfwWindowRef == nullptr)
    {
      std::cout << "Window was nullptr, aborting." << "\n";
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
      std::cout << "Failed to load a graphics context!" << "\n";
      exit(EXIT_FAILURE);
    }
    auto vendor = glGetString(GL_VENDOR);
    this->contextInfo += std::string("Graphics device vendor: ") +
                         std::string(reinterpret_cast<const char*>(vendor));
    std::cout << "Graphics device vendor: " << vendor << "\n";
    auto device = glGetString(GL_RENDERER);
    this->contextInfo += std::string("\nGraphics device: ") +
                         std::string(reinterpret_cast<const char*>(device));
    std::cout << "Graphics device: " << device << "\n";
    auto version = glGetString(GL_VERSION);
    this->contextInfo += std::string("\nGraphics context version: ") +
                         std::string(reinterpret_cast<const char*>(version));
    std::cout << "Graphics context version: " << version << "\n";

    // Check the version of the OpenGL context. Requires 4.6 or greater.
    this->versionMajor = static_cast<uint>(version[0] - 48);
    this->versionMinor = static_cast<uint>(version[2] - 48);
    if (this->versionMajor < 4)
    {
      std::cout << "Unsupported OpenGL version, application requires OpenGL "
                << "core 4.6." << "\n";
      exit(EXIT_FAILURE);
    }
    else if (this->versionMajor == 4 && this->versionMinor < 6)
    {
      std::cout << "Unsupported OpenGL version, application requires OpenGL "
                << "core 4.6." << "\n";
      exit(EXIT_FAILURE);
    }

    // Get device information.
    int totalTextureUnits, totalUniformBlocks;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &totalTextureUnits);
    this->stageTextureUnits = totalTextureUnits / 6;
    this->contextInfo += std::string("\nNumber of texture units per shader stage: ") 
                      + std::to_string(this->stageTextureUnits);

    glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &totalUniformBlocks);
    this->stageUniformBuffers = totalUniformBlocks / 6;
    this->contextInfo += std::string("\nNumber of uniform blocks per shader stage: ") 
                      + std::to_string(this->stageUniformBuffers);

    // Check the device information to ensure that shaders will compile properly.

    // Check to make sure all required extensions are supported.
    /*
    if (!GLAD_GL_ARB_bindless_texture || !GLAD_GL_ARB_indirect_parameters)
    {
      std::cout << "Missing one of the required OpenGL extensions." << "\n";
      exit(EXIT_FAILURE);
    }
    */

    std::cout.flush();
  }

  void
  GraphicsContext::swapBuffers()
  {
    glfwSwapBuffers(this->glfwWindowRef);
  }
}
