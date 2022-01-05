// Include guard.
#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Graphics/ShadingPrimatives.h"

namespace Strontium
{
  enum class EditorCameraType { Stationary, Free };

  class EditorCamera
  {
  public:
    // Constructors.
    EditorCamera(float xCenter, float yCenter, EditorCameraType type);
    EditorCamera(float xCenter, float yCenter, const glm::vec3 &initPosition,
                 EditorCameraType type);

    // Destructor.
    ~EditorCamera() = default;

    // Implement init.
    void init(const float &fov = 90.0f, const float &aspect = 1.0f,
              const float &near = 1.0f, const float &far = 200.0f);

    // Function to zoom the camera.
    void cameraZoom(glm::vec2 offsets);

    // The update function.
    void onUpdate(float dt, const glm::vec2 &viewportSize);

    // Implement the event system.
    void onEvent(Event& event);

    // Swap the camera types.
    void swap();

    // Update the projection matrx.
    void updateProj(float fov, float aspect, float near, float far);

    // Get the view/projection matrices.
    glm::mat4& getViewMatrix();
    glm::mat4& getProjMatrix();

    // Get the camera position and front.
    glm::vec3 getCamPos();
    glm::vec3 getCamFront();

    // Get the perspective parameters.
    float& getHorFOV() { return this->horFOV; }
    float& getNear() { return this->near; }
    float& getFar() { return this->far; }
    float& getAspect() { return this->aspect; }
    bool isStationary() { return this->currentType == EditorCameraType::Stationary; }

    float& getSpeed() { return this->scalarSpeed; }
    float& getSens() { return this->sensitivity; }

    operator Camera()
    {
      Camera outCam;
      outCam.fov = this->horFOV;
      outCam.near = this->near;
      outCam.far = this->far;
      outCam.view = this->view;
      outCam.projection = this->proj;
      outCam.invViewProj = glm::inverse(this->proj * this->view);
      outCam.position = this->position;
      outCam.front = this->camFront;

      return outCam;
    }
  protected:
    // Event handling functions.
    void onMouseScroll(MouseScrolledEvent &mouseEvent);
    void onKeyPress(KeyPressedEvent &keyEvent);

    // Variables for camera position, front and top vectors.
    glm::vec3 position;
    glm::vec3 pivot;
    glm::vec3 camFront;
    glm::vec3 camTop;

    // The camera matrices. Stored here to avoid recalculation on each frame.
    glm::mat4 view;
    glm::mat4 proj;

    // Previous x and y coords of the mouse, plus a boolean to detect if the app
    // just launched.
    float lastMouseX;
    float lastMouseY;
    bool firstClick;

    // FPS/free camera variables.
    float yaw;
    float pitch;
    float horFOV;
    float near;
    float far;
    float aspect;
    float scalarSpeed = 2.5f;
    float sensitivity = 0.1f;

    EditorCameraType currentType;
  };
}
