// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

class WindowItem
{
public:
  // Virtual function for keyboard/mouse input.
  virtual void keyboardAction(GLFWwindow *window) = 0;
  virtual void mouseAction(GLFWwindow *window)    = 0;
};
