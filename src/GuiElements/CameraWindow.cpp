#include "GuiElements/CameraWindow.h"

// Project includes.
#include "Graphics/Camera.h"
#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  CameraWindow::CameraWindow(Shared<Camera> camera)
    : GuiWindow()
    , camera(camera)
  { }

  CameraWindow::~CameraWindow()
  { }

  void
  CameraWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    auto& fov = this->camera->getHorFOV();
    auto& near = this->camera->getNear();
    auto& far = this->camera->getFar();
    auto& aspect = this->camera->getAspect();

    auto& speed = this->camera->getSpeed();
    auto& sense = this->camera->getSens();

    ImGui::Begin("Editor Camera Settings", &isOpen);
    ImGui::Text("Perspective Settings");
    Styles::drawFloatControl("FOV", 90.0f, fov);
    Styles::drawFloatControl("Near", 0.1f, near);
    Styles::drawFloatControl("Far", 200.0f, far);
    ImGui::Text("");
    ImGui::Text("Speed and Sentitivity");
    Styles::drawFloatControl("Speed", 2.5f, speed);
    Styles::drawFloatControl("Sensitivity", 0.1f, sense);
    ImGui::End();

    this->camera->updateProj(fov, aspect, near, far);
  }

  void
  CameraWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  CameraWindow::onEvent(Event &event)
  {

  }
}
