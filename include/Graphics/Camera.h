// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "Core/InputItem.h"

namespace SciRenderer
{
  enum CameraType { EDITOR, FPS };

  class Camera : public InputItem
  {
  public:
    // Constructors.
    Camera(float xCenter, float yCenter, CameraType type);
    Camera(float xCenter, float yCenter, const glm::vec3 &initPosition,
           CameraType type);

    // Destructor.
    ~Camera() = default;

    // Implement init.
    void init(GLFWwindow *window, const glm::mat4 &viewportProj);

    // Implement the action system.
    void keyboardAction(GLFWwindow *window);
    void mouseAction(GLFWwindow *window);
    void scrollAction(GLFWwindow *window, double xoffset, double yoffset);

    // Swap the camera types.
    void swap(GLFWwindow *window);

    // Update the projection matrx.
    void updateProj(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far);

    // Get the view/projection matrices.
    glm::mat4 getViewMatrix();
    glm::mat4 getProjMatrix();

    // Get the camera position and front.
    glm::vec3 getCamPos();
    glm::vec3 getCamFront();

  protected:
    // Cursors.
    GLFWcursor* hand;
    GLFWcursor* arrow;
    // Variables for camera position, front and top vectors.
    glm::vec3   position;
    glm::vec3   camFront;
    glm::vec3   camTop;

    glm::vec3   initPosition;
    glm::vec3   initCamFront;
    glm::vec3   initCamTop;

    // The camera matrices. Stored here to avoid recalculation on each frame.
    glm::mat4   view;
    glm::mat4   proj;

    // Time steps to normalize camera movement to frame time.
    float       dt;
    float       lastTime;
    float       currentTime;

    // Previous x and y coords of the mouse, plus a boolean to detect if the app
    // just launched.
    bool        isInit;
    float       lastMouseX;
    float       lastMouseY;

    // FPS/free camera variables.
    float       yaw;
    float       pitch;
    const float scalarSpeed = 2.5f;
    const float sensitivity = 0.1f;

    CameraType  currentType;

    // Editor cam variables.
    bool        firstClick;
  };
}
