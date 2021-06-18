#include "GuiElements/RendererWindow.h"

// Project includes.
#include "Graphics/Renderer.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  RendererWindow::RendererWindow()
    : GuiWindow()
  {

  }

  RendererWindow::~RendererWindow()
  {

  }

  void
  RendererWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    auto state = Renderer3D::getState();
    int skyboxWidth = state->skyboxWidth;
    int irradianceWidth = state->irradianceWidth;
    int prefilterWidth = state->prefilterWidth;
    int prefilterSamples = state->prefilterSamples;

    ImGui::Begin("Renderer Settings", &isOpen);

    if (ImGui::CollapsingHeader("Environment Maps"))
    {
      if (ImGui::Button("Recompute Environment Map"))
      {
        auto ambient = Renderer3D::getStorage()->currentEnvironment.get();

        ambient->unloadComputedMaps();
        ambient->equiToCubeMap(true, state->skyboxWidth, state->skyboxWidth);
        ambient->precomputeIrradiance(state->irradianceWidth, state->irradianceWidth, true);
        ambient->precomputeSpecular(state->prefilterWidth, state->prefilterWidth, true);
      }

      if (ImGui::InputInt("Skybox Size", &skyboxWidth))
      {
        skyboxWidth = skyboxWidth > 2048 ? 2048 : skyboxWidth;
        skyboxWidth = skyboxWidth < 512 ? 512 : skyboxWidth;
        skyboxWidth = std::pow(2, std::floor(std::log2(skyboxWidth)));
        state->skyboxWidth = skyboxWidth;
      }

      if (ImGui::InputInt("Irradiance Map Quality", &irradianceWidth))
      {
        irradianceWidth = irradianceWidth > 256 ? 256 : irradianceWidth;
        irradianceWidth = irradianceWidth < 64 ? 64 : irradianceWidth;
        irradianceWidth = std::pow(2, std::floor(std::log2(irradianceWidth)));
        state->irradianceWidth = irradianceWidth;
      }

      if (ImGui::InputInt("IBL Prefilter Quality", &prefilterWidth))
      {
        prefilterWidth = prefilterWidth > 2048 ? 2048 : prefilterWidth;
        prefilterWidth = prefilterWidth < 512 ? 512 : prefilterWidth;
        prefilterWidth = std::pow(2, std::floor(std::log2(prefilterWidth)));
        state->prefilterWidth = prefilterWidth;
      }

      if (ImGui::InputInt("IBL Prefilter Samples", &prefilterSamples))
      {
        prefilterSamples = prefilterSamples > 2048 ? 2048 : prefilterSamples;
        prefilterSamples = prefilterSamples < 512 ? 512 : prefilterSamples;
        prefilterSamples = std::pow(2, std::floor(std::log2(prefilterSamples)));
        state->prefilterSamples = prefilterSamples;
      }
    }
    ImGui::End();
  }

  void
  RendererWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  RendererWindow::onEvent(Event &event)
  {

  }
}
