#include "GuiElements/CameraWindow.h"

// Project includes.
#include "Graphics/Renderer.h"
#include "Graphics/RenderPasses/PostProcessingPass.h"

#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Strontium
{
  CameraWindow::CameraWindow(EditorLayer* parentLayer, EditorCamera* camera)
    : GuiWindow(parentLayer)
    , camera(camera)
  { }

  CameraWindow::~CameraWindow()
  { }

  void
  CameraWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    ImGui::Begin("Viewport Settings", &isOpen);
    if (ImGui::CollapsingHeader("Editor Camera Settings"))
    {
      Styles::drawFloatControl("FOV", 90.0f, this->camera->getHorFOV());
      Styles::drawFloatControl("Near", 0.1f, this->camera->getNear());
      Styles::drawFloatControl("Far", 200.0f, this->camera->getFar());
      Styles::drawFloatControl("Speed", 2.5f, this->camera->getSpeed());
    }
    
    if (ImGui::CollapsingHeader("Editor Viewport Settings"))
    {
      auto& passManager3D  = Renderer3D::getPassManager();
      auto postBlock = passManager3D.getRenderPass<PostProcessingPass>()->getInternalDataBlock<PostProcessingPassDataBlock>();

      ImGui::Checkbox("Draw Grid", &postBlock->useGrid);
    }
    ImGui::End();

    this->camera->updateProj(this->camera->getHorFOV(), this->camera->getAspect(), 
                             this->camera->getNear(), this->camera->getFar());
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
