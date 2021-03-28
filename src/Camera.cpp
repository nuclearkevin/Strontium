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
  , proj(glm::mat4(1.0f))
  , dt(0.0f)
  , lastTime(0.0f)
  , currentTime(0.0f)
  , isInit(false)
  , lastMouseX(xCenter)
  , lastMouseY(yCenter)
  , yaw(-90.0f)
  , pitch(0.0f)
  , currentType(type)
  , firstClick(true)
{
  this->view = glm::lookAt(this->position, this->position + this->camFront,
                           this->camTop);
  this->hand  = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
  this->arrow = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
}

Camera::Camera(float xCenter, float yCenter, const glm::vec3 &initPosition,
               CameraType type)
  : position(initPosition)
  , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
  , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
  , initPosition(initPosition)
  , initCamFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
  , initCamTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
  , proj(glm::mat4(1.0f))
  , dt(0.0f)
  , lastTime(0.0f)
  , currentTime(0.0f)
  , isInit(false)
  , lastMouseX(xCenter)
  , lastMouseY(yCenter)
  , yaw(-90.0f)
  , pitch(0.0f)
  , currentType(type)
  , firstClick(true)
{
  this->view = glm::lookAt(this->position, this->position + this->camFront,
                           this->camTop);
  this->hand  = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
  this->arrow = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
}

void
Camera::init(GLFWwindow *window, const glm::mat4 &viewportProj)
{
  this->proj = viewportProj;
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
      if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
      {
        recomputeView = true;
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(-cameraSpeed * 10),
                                    this->camFront);
        this->camTop = glm::vec3(glm::normalize(rot * glm::vec4(this->camTop, 0.0f)));
      }
      if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
      {
        recomputeView = true;
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(cameraSpeed * 10),
                                    this->camFront);
        this->camTop = glm::vec3(glm::normalize(rot * glm::vec4(this->camTop, 0.0f)));
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
  float dx = (float) this->lastMouseX - mouseX;
  float dy = (float) this->lastMouseY - mouseY;

  switch (this->currentType)
  {
    case FPS:
      // Update the camera pointing vector if the mouse position has changed.
      if (mouseX != this->lastMouseX || mouseY != this->lastMouseY)
      {
        recomputeView = true;

        // Compute the yaw and pitch from mouse position.
        this->yaw   += (this->sensitivity * (-dx));
        this->pitch += (this->sensitivity * dy);

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
      // Poll for mouse input and see if a mouse button is pressed.
      int lMBState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
      int rMBState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

      if ((lMBState == GLFW_PRESS || rMBState == GLFW_PRESS)
          && this->firstClick == true
          && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
      {
        // Activate camera motion mode.
        if (mouseX != this->lastMouseX || mouseY != this->lastMouseY)
        {
          glfwSetCursor(window, this->hand);

          this->firstClick = false;

          this->lastMouseX = mouseX;
          this->lastMouseY = mouseY;
        }
      }
      else if (lMBState == GLFW_PRESS && this->firstClick == false
               && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
      {
        if (mouseX != this->lastMouseX || mouseY != this->lastMouseY)
        {
          // Update the camera matrix if the mouse is moving AND LMB is pressed.
          recomputeView = true;

          // Compute the rotation variables and matrix.
          // This gimble locks. TODO figure out quaternions to do this properly.
          glm::mat4 rot;
          glm::vec3 camRight = glm::normalize(glm::cross(this->camFront, this->camTop));
          rot = glm::rotate(glm::mat4(1.0f), glm::radians(dx), this->camTop);
          rot = glm::rotate(rot, glm::radians(dy), camRight);
          glm::mat4 rotNorm = glm::transpose(glm::inverse(rot));
          this->position = glm::vec3(rot * glm::vec4(this->position, 1.0f));
          this->camFront = glm::normalize(glm::vec3(rotNorm * glm::vec4(this->camFront, 0.0f)));
          this->camTop   = glm::normalize(glm::vec3(rotNorm * glm::vec4(this->camTop, 0.0f)));

          this->lastMouseX = mouseX;
          this->lastMouseY = mouseY;
        }
      }
      else if (rMBState == GLFW_PRESS && this->firstClick == false
               && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
      {
        if (mouseX != this->lastMouseX || mouseY != this->lastMouseY)
        {
          // Update the camera matrix if the mouse is moving AND LMB is pressed.
          recomputeView = true;

          // Compute the translation.
          glm::mat4 transX, transY, transTot;
          glm::vec3 camRight = glm::normalize(glm::cross(this->camFront, this->camTop))
                               * dx * this->sensitivity;
          transX = glm::translate(glm::mat4(1.0f), camRight);
          transY = glm::translate(glm::mat4(1.0f), this->camTop * dy * this->sensitivity);
          transTot = transY * transX;
          glm::mat4 transNorm = glm::transpose(glm::inverse(transTot));
          this->position = glm::vec3(transTot * glm::vec4(this->position, 1.0f));
          this->camFront = glm::vec3(transNorm * glm::vec4(this->camFront, 0.0f));

          this->lastMouseX = mouseX;
          this->lastMouseY = mouseY;
        }
      }
      else if ((lMBState == GLFW_RELEASE || rMBState == GLFW_RELEASE) && this->firstClick == false)
      {
        this->firstClick = true;
        glfwSetCursor(window, this->arrow);
      }
      break;
  }
  // Recompute the view matrix if required.
  if (recomputeView)
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
}

// Scroll callback.
void
Camera::scrollAction(GLFWwindow *window, double xoffset, double yoffset)
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
      if (yoffset > 0.0 && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
      {
        recomputeView = true;
        this->position += this->camFront * cameraSpeed;
      }
      else if (yoffset < 0.0 && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
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

// Update the projection matrix.
void
Camera::updateProj(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far)
{
  this->proj = glm::perspective(glm::radians(fov), aspect, near, far);
}

// Fetch the view/projection matrix of the camera.
glm::mat4
Camera::getViewMatrix()
{
  return this->view;
}

glm::mat4
Camera::getProjMatrix()
{
  return this->proj;
}

// Fetch the camera position (for shading).
glm::vec3
Camera::getCamPos()
{
  return this->position;
}

// Fetch the camera front vector (for shading).
glm::vec3
Camera::getCamFront()
{
  return this->camFront;
}
