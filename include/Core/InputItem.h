// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

namespace SciRenderer
{
  class InputItem
  {
  public:
    // Virtual function for keyboard/mouse input.
    virtual void keyboardAction(GLFWwindow *window)           = 0;
    virtual void mouseAction(GLFWwindow *window)              = 0;
    virtual void scrollAction(GLFWwindow *window,
                              double xoffset, double yoffset) = 0;
  };
}
