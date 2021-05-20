// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"

namespace SciRenderer
{
  enum class EditorCameraType { Stationary, Free };

  class Camera
  {
  public:
    // Constructors.
    Camera(float xCenter, float yCenter, EditorCameraType type);
    Camera(float xCenter, float yCenter, const glm::vec3 &initPosition,
           EditorCameraType type);

    // Destructor.
    ~Camera() = default;

    // Implement init.
    void init(const GLfloat &fov = 90.0f, const GLfloat &aspect = 1.0f,
              const GLfloat &near = 1.0f, const GLfloat &far = 30.0f);

    // Function to zoom the camera.
    void cameraZoom(glm::vec2 offsets);

    // The update function.
    void onUpdate(float dt);

    // Implement the event system.
    void onEvent(Event& event);

    // Swap the camera types.
    void swap();

    // Update the projection matrx.
    void updateProj(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far);

    // Get the view/projection matrices.
    glm::mat4 getViewMatrix();
    glm::mat4 getProjMatrix();

    // Get the camera position and front.
    glm::vec3 getCamPos();
    glm::vec3 getCamFront();

  protected:
    // Event handling functions.
    void onMouseScroll(MouseScrolledEvent &mouseEvent);
    void onWindowResize(WindowResizeEvent &windowEvent);
    void onKeyPress(KeyPressedEvent &keyEvent);

    // Variables for camera position, front and top vectors.
    glm::vec3   position;
    glm::vec3   camFront;
    glm::vec3   camTop;

    // The camera matrices. Stored here to avoid recalculation on each frame.
    glm::mat4   view;
    glm::mat4   proj;

    // Time steps to normalize camera movement to frame time.
    float       lastTime;

    // Previous x and y coords of the mouse, plus a boolean to detect if the app
    // just launched.
    float       lastMouseX;
    float       lastMouseY;

    // FPS/free camera variables.
    float       yaw;
    float       pitch;
    const float scalarSpeed = 2.5f;
    const float sensitivity = 0.1f;

    EditorCameraType  currentType;

    // Editor cam variables.
    bool        firstClick;
  };
}
