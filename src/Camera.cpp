// Project includes.
#include "Camera.h"

using namespace SciRenderer;

// Constructors.
Camera::Camera(float xCenter, float yCenter, CameraType type)
  : position(glm::vec3 { 0.0f, 0.0f, 0.0f })
  , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
  , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
  , initPosition(glm::vec3 { 0.0f, 0.0f, 0.0f })
  , initCamFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
  , initCamTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
  , dt(0.0f)
  , lastTime(0.0f)
  , currentTime(0.0f)
  , isInit(false)
  , lastMouseX(xCenter)
  , lastMouseY(yCenter)
  , yaw(-90.0f)
  , pitch(0.0f)
  , currentType(type)
{
  this->view = glm::lookAt(this->position, this->position + this->camFront,
                           this->camTop);
}

Camera::Camera(float xCenter, float yCenter,
                            const glm::vec3 &initPosition, CameraType type)
  : position(initPosition)
  , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
  , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
  , initPosition(initPosition)
  , initCamFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
  , initCamTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
  , dt(0.0f)
  , lastTime(0.0f)
  , currentTime(0.0f)
  , isInit(false)
  , lastMouseX(xCenter)
  , lastMouseY(yCenter)
  , yaw(-90.0f)
  , pitch(0.0f)
  , currentType(type)
{
  this->view = glm::lookAt(this->position, this->position + this->camFront,
                           this->camTop);
}

void
Camera::init(GLFWwindow *window)
{
  switch (this->currentType)
  {
    case FPS:
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      this->isInit = true;
      break;
    case EDITOR:
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      this->isInit = true;
      break;
  }
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

  switch (this->currentType)
  {
    case FPS:
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
      break;
    case EDITOR:
      break;
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

  switch (this->currentType)
  {
    case FPS:
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
      break;
    case EDITOR:
      break;
  }
  // Recompute the view matrix if required.
  if (recomputeView)
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
}

// Scroll callback.
void
Camera::scrollAction(double xoffset, double yoffset)
{
  bool recomputeView = false;

  // Timestep for camera movement.
  this->currentTime = glfwGetTime();
  this->dt = this->currentTime - this->lastTime;
  this->lastTime = this->currentTime;

  float cameraSpeed = 200 * ((float) yoffset) * this->scalarSpeed * this->dt;

  switch (this->currentType)
  {
    case FPS:
      break;
    case EDITOR:
      if (yoffset > 0.0)
      {
        recomputeView = true;
        this->position += this->camFront * cameraSpeed;
      }
      else if (yoffset < 0.0)
      {
        recomputeView = true;
        this->position += this->camFront * cameraSpeed;
      }
      break;
  }
  // Recompute the view matrix if required.
  if (recomputeView)
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
}

// Swap the camera types.
void
Camera::swap(GLFWwindow *window)
{
  if (this->currentType == EDITOR)
  {
    this->currentType = FPS;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }
  else
  {
    this->currentType = EDITOR;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  double mouseX, mouseY;
  glfwGetCursorPos(window, &mouseX, &mouseY);
  this->lastMouseX = (float) mouseX;
  this->lastMouseY = (float) mouseY;
}

// Fetch the view matrix of the camera.
glm::mat4*
Camera::getViewMatrix()
{
  return &(this->view);
}

// Fetch the camera position (for shading).
glm::vec4
Camera::getCamPos()
{
  return glm::vec4(this->position[0], this->position[1], this->position[2], 1.0f);
}

// Fetch the camera front vector (for shading).
glm::vec3
Camera::getCamFront()
{
  return this->camFront;
}
