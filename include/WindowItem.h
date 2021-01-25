// Include guard.
#pragma once

// OpenGL includes.
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class WindowItem
{
public:
  // Virtual function for keyboard/mouse input.
  virtual void keyboardAction(GLFWwindow *window) = 0;
  virtual void mouseAction(GLFWwindow *window)    = 0;
};
