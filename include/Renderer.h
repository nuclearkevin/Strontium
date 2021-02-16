// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "VertexArray.h"
#include "Shaders.h"

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
  void draw(GLFWwindow* window, VertexArray* data, Shader* program);

  void swap(GLFWwindow* window);

private:
  // Constructor.
  Renderer() = default;
};
