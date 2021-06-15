#include "Graphics/Camera.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/Application.h"
#include "Core/Window.h"

namespace SciRenderer
{
  Camera::Camera(GLfloat xCenter, GLfloat yCenter, EditorCameraType type)
    : position(glm::vec3 { 0.0f, 0.0f, 0.0f })
    , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
    , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
    , proj(glm::mat4(1.0f))
    , lastTime(0.0f)
    , lastMouseX(xCenter)
    , lastMouseY(yCenter)
    , yaw(-90.0f)
    , pitch(0.0f)
    , currentType(type)
    , firstClick(true)
  {
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
  }

  Camera::Camera(GLfloat xCenter, GLfloat yCenter, const glm::vec3 &initPosition,
                 EditorCameraType type)
    : position(initPosition)
    , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
    , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
    , proj(glm::mat4(1.0f))
    , lastTime(0.0f)
    , lastMouseX(xCenter)
    , lastMouseY(yCenter)
    , yaw(-90.0f)
    , pitch(0.0f)
    , currentType(type)
    , firstClick(true)
  {
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
  }

  void
  Camera::init(const GLfloat &fov, const GLfloat &aspect,
               const GLfloat &near, const GLfloat &far)
  {
    this->proj = glm::perspective(glm::radians(fov), aspect, near, far);
    switch (this->currentType)
    {
      case EditorCameraType::Free:
        Application::getInstance()->getWindow()->setCursorCapture(true);
        break;
      case EditorCameraType::Stationary:
        Application::getInstance()->getWindow()->setCursorCapture(false);
        break;
    }

    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();
    glm::vec2 mousePos = appWindow->getCursorPos();
    this->lastMouseX = mousePos.x;
    this->lastMouseY = mousePos.y;

    this->horFOV = fov;
    this->near = near;
    this->far = far;
    this->aspect = aspect;
  }

  // On update function for the camera.
  void
  Camera::onUpdate(float dt)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    glm::vec2 mousePos = appWindow->getCursorPos();

    switch (this->currentType)
    {
      case EditorCameraType::Stationary:
      {
        break;
      }
      case EditorCameraType::Free:
      {
        //----------------------------------------------------------------------
        // Handles the mouse input component.
        //----------------------------------------------------------------------
        GLfloat dx = mousePos.x - this->lastMouseX;
        GLfloat dy = this->lastMouseY - mousePos.y;

        // Compute the yaw and pitch from mouse position.
        this->yaw   += (this->sensitivity * dx);
        this->pitch += (this->sensitivity * dy);

        if (this->pitch > 89.0f)
          this-> pitch = 89.0f;
        if (this->pitch < -89.0f)
          this->pitch = -89.0f;

        // Compute the new front normal vector.
        glm::vec3 temp;
        temp.x         = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
        temp.y         = sin(glm::radians(this->pitch));
        temp.z         = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
        this->camFront = glm::normalize(temp);

        //----------------------------------------------------------------------
        // Handles the keyboard input component.
        //----------------------------------------------------------------------
        GLfloat cameraSpeed = this->scalarSpeed * dt;

        // Move the camera position (Space Engineers styled camera).
        if (appWindow->isKeyPressed(GLFW_KEY_W))
          this->position += this->camFront * cameraSpeed;

        if (appWindow->isKeyPressed(GLFW_KEY_S))
          this->position -= this->camFront * cameraSpeed;

        if (appWindow->isKeyPressed(GLFW_KEY_A))
          this->position -= glm::normalize(glm::cross(this->camFront, this->camTop))
                            * cameraSpeed;

        if (appWindow->isKeyPressed(GLFW_KEY_D))
          this->position += glm::normalize(glm::cross(this->camFront, this->camTop))
                            * cameraSpeed;

        if (appWindow->isKeyPressed(GLFW_KEY_SPACE))
          this->position += this->camTop * cameraSpeed;

        if (appWindow->isKeyPressed(GLFW_KEY_LEFT_CONTROL))
          this->position -= this->camTop * cameraSpeed;

        if (appWindow->isKeyPressed(GLFW_KEY_Q))
        {
          glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(-cameraSpeed * 10),
                                      this->camFront);
          this->camTop = glm::vec3(glm::normalize(rot * glm::vec4(this->camTop, 0.0f)));
        }
        if (appWindow->isKeyPressed(GLFW_KEY_E))
        {
          glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(cameraSpeed * 10),
                                      this->camFront);
          this->camTop = glm::vec3(glm::normalize(rot * glm::vec4(this->camTop, 0.0f)));
        }
        break;
      }
    }

    // Recompute the view matrix.
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);

    this->lastMouseX = mousePos.x;
    this->lastMouseY = mousePos.y;
  }

  // Camera zoom function.
  void
  Camera::cameraZoom(glm::vec2 offsets)
  {
    GLfloat cameraSpeed = 0.02 * (offsets.y) * this->scalarSpeed;

    this->position += this->camFront * cameraSpeed;

    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
  }

  // The event handling function.
  void
  Camera::onEvent(Event &event)
  {
    switch (event.getType())
    {
      case EventType::MouseScrolledEvent:
        this->onMouseScroll(*(static_cast<MouseScrolledEvent*>(&event)));
        break;
      case EventType::WindowResizeEvent:
        this->onWindowResize(*(static_cast<WindowResizeEvent*>(&event)));
        break;
      case EventType::KeyPressedEvent:
        this->onKeyPress(*(static_cast<KeyPressedEvent*>(&event)));
      default:
        break;
    }
  }

  void
  Camera::onMouseScroll(MouseScrolledEvent &mouseEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window>appWindow = Application::getInstance()->getWindow();

    glm::vec2 offsets = mouseEvent.getOffset();

    switch (this->currentType)
    {
      case EditorCameraType::Stationary:
        if (offsets.y != 0.0 && appWindow->isKeyPressed(GLFW_KEY_LEFT_ALT))
          this->cameraZoom(offsets);
        break;
      default:
        break;
    }
  }

  void
  Camera::onWindowResize(WindowResizeEvent &windowEvent)
  {

  }

  void
  Camera::onKeyPress(KeyPressedEvent &keyEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int keyCode = keyEvent.getKeyCode();

    if (keyCode == GLFW_KEY_P && appWindow->isKeyPressed(GLFW_KEY_LEFT_ALT))
      this->swap();
  }

  // Swap the camera types.
  void
  Camera::swap()
  {
    Logger* logs = Logger::getInstance();
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    if (this->currentType == EditorCameraType::Stationary)
    {
      this->currentType = EditorCameraType::Free;
      appWindow->setCursorCapture(true);
      logs->logMessage(LogMessage("Swapped camera to free-form.", true, false, false));
    }
    else
    {
      this->currentType = EditorCameraType::Stationary;
      appWindow->setCursorCapture(false);
      logs->logMessage(LogMessage("Swapped camera to stationary.", true, false, false));
    }

    glm::vec2 cursorPos = appWindow->getCursorPos();
    this->lastMouseX = cursorPos.x;
    this->lastMouseY = cursorPos.y;
  }

  // Update the projection matrix.
  void
  Camera::updateProj(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far)
  {
    this->proj = glm::perspective(glm::radians(fov), aspect, near, far);

    this->horFOV = fov;
    this->near = near;
    this->far = far;
    this->aspect = aspect;
  }

  // Fetch the view/projection matrix of the camera.
  glm::mat4&
  Camera::getViewMatrix()
  {
    return this->view;
  }

  glm::mat4&
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
}
