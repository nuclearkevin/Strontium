// Project includes.
#include "Camera.h"

// OpenGL includes.
#include <glm/gtc/matrix_transform.hpp>

// STL and standard includes.
#include <math.h>
#include <iostream>

// Constructors.
Camera::Camera(float xCenter, float yCenter)
  : position(glm::vec3 { 0.0f, 0.0f, 0.0f })
  , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
  , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
  , dt(0.0f)
  , lastTime(0.0f)
  , currentTime(0.0f)
  , init(false)
  , lastMouseX(xCenter)
  , lastMouseY(yCenter)
  , yaw(-90.0f)
  , pitch(0.0f)
{
  this->view = glm::lookAt(this->position, this->position + this->camFront,
                           this->camTop);
}

Camera::Camera(float xCenter, float yCenter, const glm::vec3 &initPosition)
  : position(initPosition)
  , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
  , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
  , dt(0.0f)
  , lastTime(0.0f)
  , currentTime(0.0f)
  , init(false)
  , lastMouseX(xCenter)
  , lastMouseY(yCenter)
  , yaw(-90.0f)
  , pitch(0.0f)
{
  this->view = glm::lookAt(this->position, this->position + this->camFront,
                           this->camTop);
}

// Action system for camera control.
void
Camera::keyboardAction(GLFWwindow *window)
{
  // Boolean to avoid recomputing the view matrix every frame if the camera
  // hasn't changed.
  bool recomputeView = false;

  // Timestep for camera movement.
  this->currentTime = glfwGetTime();
  this->dt = this->currentTime - this->lastTime;
  this->lastTime = this->currentTime;

  float cameraSpeed = this->scalarSpeed * this->dt;

  // Move the camera position (Space Engineers styled camera).
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    recomputeView = true;
    this->position += this->camFront * cameraSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    recomputeView = true;
    this->position -= this->camFront * cameraSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    recomputeView = true;
    this->position -= glm::normalize(glm::cross(this->camFront, this->camTop))
                      * cameraSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    recomputeView = true;
    this->position += glm::normalize(glm::cross(this->camFront, this->camTop))
                      * cameraSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
  {
    recomputeView = true;
    this->position += this->camTop * cameraSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
  {
    recomputeView = true;
    this->position -= this->camTop * cameraSpeed;
  }

  // Recompute the view matrix if required.
  if (recomputeView)
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
}

void
Camera::mouseAction(GLFWwindow *window)
{
  bool recomputeView = false;

  double mouseX, mouseY;
  glfwGetCursorPos(window, &mouseX, &mouseY);

  // Update the camera pointing vector if the mouse position has changed.
  if (mouseX != this->lastMouseX || mouseY != this->lastMouseY)
  {
    recomputeView = true;

    // Compute the yaw and pitch from mouse position.
    this->yaw   += (this->sensitivity * (mouseX - this->lastMouseX));
    this->pitch += (this->sensitivity * (this->lastMouseY - mouseY));

    if (this->pitch > 89.0f)
      this-> pitch = 89.0f;
    if (this->pitch < -89.0f)
      this->pitch = -89.0f;

    // Compute the new front normal vector.
    glm::vec3 temp;
    temp[0]          = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
    temp[1]          = sin(glm::radians(this->pitch));
    temp[2]          = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
    this->camFront   = glm::normalize(temp);

    this->lastMouseX = mouseX;
    this->lastMouseY = mouseY;
  }

  // Recompute the view matrix if required.
  if (recomputeView)
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
}

// Fetch the view matrix of the camera.
glm::mat4*
Camera::getViewMatrix()
{
  return &(this->view);
}
