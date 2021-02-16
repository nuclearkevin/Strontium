// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "InputItem.h"

enum CameraType { EDITOR, FPS };

class Camera : public InputItem
{
public:
  // Constructors.
  Camera(float xCenter, float yCenter, CameraType type);
  Camera(float xCenter, float yCenter, const glm::vec3 &initPosition, CameraType type);

  // Destructor.
  ~Camera() = default;

  // Implement init.
  void init(GLFWwindow *window);

  // Implement the action system.
  void keyboardAction(GLFWwindow *window);
  void mouseAction(GLFWwindow *window);
  void scrollAction(double xoffset, double yoffset);

  // Swap the camera types.
  void swap(GLFWwindow *window);

  // Get the view matrix.
  glm::mat4* getViewMatrix();

  // Get the camera position and front.
  glm::vec4 getCamPos();
  glm::vec3 getCamFront();

protected:
  // Variables for camera position, front and top vectors.
  glm::vec3   position;
  glm::vec3   camFront;
  glm::vec3   camTop;

  glm::vec3   initPosition;
  glm::vec3   initCamFront;
  glm::vec3   initCamTop;

  // Camera view matrix, stored here to avoid recomputing the matrix each time.
  glm::mat4   view;

  // Time steps to normalize camera movement to frame time.
  float       dt;
  float       lastTime;
  float       currentTime;

  // Previous x and y coords of the mouse, plus a boolean to detect if the app
  // just launched.
  bool        isInit;
  float       lastMouseX;
  float       lastMouseY;

  // Camera yaw and pitch.
  float       yaw;
  float       pitch;

  // Constants for sensitivity.
  const float scalarSpeed = 2.5f;
  const float sensitivity = 0.1f;

  CameraType currentType;
};
