// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "WindowItem.h"

class Camera : public WindowItem
{
public:
  // Constructors.
  Camera(float xCenter, float yCenter);
  Camera(float xCenter, float yCenter, const glm::vec3 &initPosition);

  // Destructor.
  ~Camera() = default;

  // Implement the action system.
  void keyboardAction(GLFWwindow *window);
  void mouseAction(GLFWwindow *window);

  // Get the view matrix.
  glm::mat4* getViewMatrix();

  // Get the camera position.
  glm::vec4 getCamPos();

  glm::vec3 getCamFront();

protected:
  // Variables for camera position, front and top vectors.
  glm::vec3   position;
  glm::vec3   camFront;
  glm::vec3   camTop;

  // Camera view matrix, stored here to avoid recomputing the matrix each time.
  glm::mat4   view;

  // Time steps to normalize camera movement to frame time.
  float       dt;
  float       lastTime;
  float       currentTime;

  // Previous x and y coords of the mouse, plus a boolean to detect if the app
  // just launched.
  bool        init;
  float       lastMouseX;
  float       lastMouseY;

  // Camera yaw and pitch.
  float       yaw;
  float       pitch;

  // Constants for sensitivity.
  const float scalarSpeed = 2.5f;
  const float sensitivity = 0.1f;
};
