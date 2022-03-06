#include "Graphics/EditorCamera.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/KeyCodes.h"

namespace Strontium
{
  EditorCamera::EditorCamera(float xCenter, float yCenter, EditorCameraType type)
    : position(0.0f, 0.0f, 1.0f)
    , pivot(0.0f, 1.0f, 0.0f)
    , camFront(glm::normalize(pivot - position))
    , camTop(0.0f, 1.0f, 0.0f)
    , proj(1.0f)
    , lastMouseX(xCenter)
    , lastMouseY(yCenter)
    , currentType(type)
    , firstClick(true)
    , horFOV(90.0f)
    , near(1.0f)
    , far(200.0f)
    , aspect(1.0f)
    , scalarSpeed(2.5f)
  {
    this->view = glm::lookAt(this->position, this->pivot,
                             this->camTop);
  }

  EditorCamera::EditorCamera(float xCenter, float yCenter, const glm::vec3 &initPosition,
                             EditorCameraType type)
    : position(initPosition)
    , pivot(0.0f, 1.0f, 0.0f)
    , camFront(glm::normalize(pivot - position))
    , camTop(0.0f, 1.0f, 0.0f)
    , proj(1.0f)
    , lastMouseX(xCenter)
    , lastMouseY(yCenter)
    , currentType(type)
    , firstClick(true)
    , horFOV(90.0f)
    , near(1.0f)
    , far(200.0f)
    , aspect(1.0f)
    , scalarSpeed(2.5f)
  {
    this->view = glm::lookAt(this->position, this->pivot,
                             this->camTop);
  }

  void
  EditorCamera::init(const float &fov, const float &aspect,
                     const float &near, const float &far)
  {
    this->proj = glm::perspective(glm::radians(fov), aspect, near, far);

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
  EditorCamera::onUpdate(float dt, const glm::vec2& viewportSize)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    glm::vec2 mousePos = appWindow->getCursorPos();

    switch (this->currentType)
    {
      //------------------------------------------------------------------------
      // Arcball camera.
      //------------------------------------------------------------------------
      case EditorCameraType::Stationary:
      {
        //----------------------------------------------------------------------
        // Handles the mouse input component.
        //----------------------------------------------------------------------
        auto right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), this->camFront));
        if (appWindow->isMouseClicked(SR_MOUSE_BUTTON_RIGHT))
        {
          float dAngleX = 2.0f * glm::pi<float>() * (this->lastMouseX - mousePos.x) / viewportSize.x;
          float dAngleY = glm::pi<float>() * (this->lastMouseY - mousePos.y) / viewportSize.y;
          
          float cosTheta = glm::dot(this->camFront, glm::vec3(0.0f, 1.0f, 0.0f));
          if (cosTheta * glm::sign(dAngleY) > 0.99f)
            dAngleY = 0.0f;
          
          auto orientation = glm::quat(glm::vec3(0.0f, 1.0f, 0.0f) + this->camFront * dAngleY, glm::vec3(0.0f, 1.0f, 0.0f));
          orientation *= glm::quat(this->camFront + right * (-dAngleX), this->camFront);
          orientation = glm::normalize(orientation);

          auto rot = glm::toMat4(orientation);
          auto tempPosition = rot * (glm::vec4(this->position, 1.0f) - glm::vec4(this->pivot, 0.0f)) + glm::vec4(this->pivot, 0.0f);

          this->position = glm::vec3(tempPosition);
          this->camFront = glm::normalize(this->pivot - this->position);
        }

        //----------------------------------------------------------------------
        // Handles the keyboard input component.
        //----------------------------------------------------------------------
        if (appWindow->isMouseClicked(SR_MOUSE_BUTTON_MIDDLE))
        {
          float dx = (this->lastMouseX - mousePos.x) / viewportSize.x;
          float dy = (this->lastMouseY - mousePos.y) / viewportSize.y;

          auto frontRelativeUp = glm::normalize(glm::cross(this->camFront, right));

          this->position += right * dx;
          this->position += frontRelativeUp * dy;
          this->pivot += right * dx;
          this->pivot += frontRelativeUp * dy;
        }

        break;
      }

      //------------------------------------------------------------------------
      // Free-form camera.
      //------------------------------------------------------------------------
      case EditorCameraType::Free:
      {
        //----------------------------------------------------------------------
        // Handles the mouse input component.
        //----------------------------------------------------------------------
        auto right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), this->camFront));
        if (appWindow->isMouseClicked(SR_MOUSE_BUTTON_RIGHT))
        {
          float dAngleX = 2.0f * glm::pi<float>() * (this->lastMouseX - mousePos.x) / viewportSize.x;
          float dAngleY = glm::pi<float>() * (this->lastMouseY - mousePos.y) / viewportSize.y;
          
          float cosTheta = glm::dot(this->camFront, glm::vec3(0.0f, 1.0f, 0.0f));
          if (cosTheta * glm::sign(dAngleY) > 0.99f)
              dAngleY = 0.0f;
          
          auto orientation = glm::quat(glm::vec3(0.0f, 1.0f, 0.0f) + this->camFront * dAngleY, glm::vec3(0.0f, 1.0f, 0.0f));
          orientation *= glm::quat(this->camFront + right * (-dAngleX), this->camFront);
          orientation = glm::normalize(orientation);
          
          auto rot = glm::toMat4(orientation);
          auto temp = glm::vec3(rot * glm::vec4(this->camFront, 0.0f));
          float distanceToPivot = glm::length(this->pivot - this->position);
          this->camFront = glm::normalize(temp);
          this->pivot = this->position + (this->camFront * distanceToPivot);
          
          //----------------------------------------------------------------------
          // Handles the keyboard input component.
          //----------------------------------------------------------------------
          float cameraSpeed = this->scalarSpeed * dt;
          if (appWindow->isKeyPressed(SR_KEY_W))
          {
            auto velocity = this->camFront * cameraSpeed;
            this->position += velocity;
            this->pivot += velocity;
          }
          
          if (appWindow->isKeyPressed(SR_KEY_S))
          {
            auto velocity = -this->camFront * cameraSpeed;
            this->position += velocity;
            this->pivot += velocity;
          }
          
          if (appWindow->isKeyPressed(SR_KEY_A))
          {
            auto velocity = -glm::normalize(glm::cross(this->camFront, this->camTop))
                             * cameraSpeed;
            this->position += velocity;
            this->pivot += velocity;
          }
          
          if (appWindow->isKeyPressed(SR_KEY_D))
          {
            auto velocity = glm::normalize(glm::cross(this->camFront, this->camTop))
                            * cameraSpeed;
            this->position += velocity;
            this->pivot += velocity;
          }
          
          if (appWindow->isKeyPressed(SR_KEY_SPACE))
          {
            auto velocity = this->camTop * cameraSpeed;
            this->position += velocity;
            this->pivot += velocity;
          }
          
          if (appWindow->isKeyPressed(SR_KEY_LEFT_CONTROL))
          {
            auto velocity = -this->camTop * cameraSpeed;
            this->position += velocity;
            this->pivot += velocity;
          }
        }

        break;
      }
    }

    this->lastMouseX = mousePos.x;
    this->lastMouseY = mousePos.y;

    // Recompute the view matrix.
    this->view = glm::lookAt(this->position, this->pivot,
                             this->camTop);
  }

  // Camera zoom function.
  void
  EditorCamera::cameraZoom(glm::vec2 offsets)
  {
    float cameraSpeed = 0.02 * (offsets.y) * this->scalarSpeed;

    this->position += this->camFront * cameraSpeed;
    this->camFront = glm::normalize(this->pivot - this->position);

    this->view = glm::lookAt(this->position, this->pivot,
                             this->camTop);
  }

  // The event handling function.
  void
  EditorCamera::onEvent(Event &event)
  {
    switch (event.getType())
    {
      case EventType::MouseScrolledEvent:
        this->onMouseScroll(*(static_cast<MouseScrolledEvent*>(&event)));
        break;
      case EventType::KeyPressedEvent:
        this->onKeyPress(*(static_cast<KeyPressedEvent*>(&event)));
        break;
      default: break;
    }
  }

  void
  EditorCamera::onMouseScroll(MouseScrolledEvent &mouseEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window>appWindow = Application::getInstance()->getWindow();

    glm::vec2 offsets = mouseEvent.getOffset();

    switch (this->currentType)
    {
      case EditorCameraType::Stationary: this->cameraZoom(offsets); break;
      default: break;
    }
  }

  void
  EditorCamera::onKeyPress(KeyPressedEvent &keyEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int keyCode = keyEvent.getKeyCode();

    if (keyCode == SR_KEY_P && appWindow->isKeyPressed(SR_KEY_LEFT_ALT))
      this->swap();
  }

  // Swap the camera types.
  void
  EditorCamera::swap()
  {
    Logger* logs = Logger::getInstance();
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    if (this->currentType == EditorCameraType::Stationary)
    {
      this->currentType = EditorCameraType::Free;
      logs->logMessage(LogMessage("Swapped camera to free-form.", true, false));
    }
    else
    {
      this->currentType = EditorCameraType::Stationary;
      logs->logMessage(LogMessage("Swapped camera to stationary.", true, false));

      glm::vec2 cursorPos = appWindow->getCursorPos();
      this->lastMouseX = cursorPos.x;
      this->lastMouseY = cursorPos.y;
    }
  }

  // Update the projection matrix.
  void
  EditorCamera::updateProj(float fov, float aspect, float near, float far)
  {
    this->proj = glm::perspective(glm::radians(fov), aspect, near, far);

    this->horFOV = fov;
    this->near = near;
    this->far = far;
    this->aspect = aspect;
  }

  // Fetch the view/projection matrix of the camera.
  glm::mat4&
  EditorCamera::getViewMatrix()
  {
    return this->view;
  }

  glm::mat4&
  EditorCamera::getProjMatrix()
  {
    return this->proj;
  }

  // Fetch the camera position (for shading).
  glm::vec3
  EditorCamera::getCamPos()
  {
    return this->position;
  }

  // Fetch the camera front vector (for shading).
  glm::vec3
  EditorCamera::getCamFront()
  {
    return this->camFront;
  }
}
