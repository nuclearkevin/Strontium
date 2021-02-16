// TODO
// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "WindowItem.h"
#include "Meshes.h"

class KeyBoardActions : public WindowItem
{
public:
  // Constructor/destructor.
  KeyBoardActions() = default;
  ~KeyBoardActions() = default;

  // Set the model to move.
  void setModel(Mesh* model);

  // Implement the action system.
  void keyboardAction(GLFWwindow *window);
  void mouseAction(GLFWwindow *window);

private:
  Mesh* model;

  float dt;
  float lastTime;
  float currentTime;
};
