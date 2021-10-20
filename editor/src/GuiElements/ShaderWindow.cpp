#include "GuiElements/ShaderWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Core/Logs.h"
#include "Graphics/Shaders.h"
#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Strontium
{
  ShaderWindow::ShaderWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , selectedShader(nullptr)
    , shaderName("")
  { }

  ShaderWindow::~ShaderWindow()
  { }

  void
  ShaderWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    Logger* logs = Logger::getInstance();
    auto shaderCache = AssetManager<Shader>::getManager();

    ImGui::Begin("Shader Programs", &isOpen);
    for (auto& name : shaderCache->getStorage())
    {
      Shader* shader = shaderCache->getAsset(name);

      ImGuiTreeNodeFlags flags = ((this->selectedShader == shader) ? ImGuiTreeNodeFlags_Selected : 0);

      ImGui::SetNextItemOpen(this->selectedShader == shader);

      bool opened = ImGui::TreeNodeEx((void*) shader, flags, name.c_str());

      if (ImGui::IsItemClicked() && this->selectedShader != shader)
      {
        this->shaderName = name;
        this->selectedShader = shader;
      }
      else if (ImGui::IsItemClicked() && this->selectedShader == shader)
      {
        this->shaderName = "";
        this->selectedShader = nullptr;
      }

      if (this->selectedShader == shader && opened)
      {
        const float buttonWidth = 140.0f;

        // Reloads the shader file.
        ImGui::Indent();
        if (ImGui::Button("Reload From File", ImVec2(buttonWidth, 0)))
        {
          logs->logMessage(LogMessage(std::string("Reloaded shader: ") + this->shaderName,
                                      true, true));
          this->selectedShader->rebuild();
        }

        // Display the shader information string.
        ImGui::Text("Shader information:");
        // TODO: Shader reflection.
        //ImGui::Text(this->selectedShader->getInfoString().c_str());
        ImGui::Unindent();

        ImGui::TreePop();
      }
      else if (opened)
        ImGui::TreePop();
    }

    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
    {
      this->shaderName = "";
      this->selectedShader = nullptr;
    }

    ImGui::End();
  }

  void
  ShaderWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  ShaderWindow::onEvent(Event &event)
  {

  }
}
