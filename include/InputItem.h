// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

class InputItem
{
public:
  // Virtual init function.
  virtual void init(GLFWwindow *window)                     = 0;

  // Virtual function for keyboard/mouse input.
  virtual void keyboardAction(GLFWwindow *window)           = 0;
  virtual void mouseAction(GLFWwindow *window)              = 0;
  virtual void scrollAction(double xoffset, double yoffset) = 0;
};
