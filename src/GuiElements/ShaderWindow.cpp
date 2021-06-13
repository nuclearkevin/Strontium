#include "GuiElements/ShaderWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Core/Logs.h"
#include "Graphics/Shaders.h"
#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
{
  ShaderWindow::ShaderWindow()
    : GuiWindow()
    , selectedShader(nullptr)
    , shaderName("")
  {
    auto glsl = TextEditor::LanguageDefinition::GLSL();
    this->vertEditor.SetLanguageDefinition(glsl);
    this->fragEditor.SetLanguageDefinition(glsl);
  }

  ShaderWindow::~ShaderWindow()
  { }

  void
  ShaderWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    Logger* logs = Logger::getInstance();
    auto shaderCache = AssetManager<Shader>::getManager();

    static bool editVert = false;
    static bool editFrag = false;

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
                                      true, false, true));
          this->selectedShader->rebuild();
        }

        // Reloads from the shader source.
        ImGui::SameLine();
        if (ImGui::Button("Reload From Source", ImVec2(buttonWidth, 0)))
        {
          logs->logMessage(LogMessage(std::string("Reloaded shader: ") + this->shaderName,
                                      true, false, true));

          std::string tempVertSource = vertEditor.GetText();
          std::string tempFragSource = fragEditor.GetText();
          if (tempVertSource.size() == 1 || tempFragSource.size() == 1)
            this->selectedShader->rebuildFromString();
          else
            this->selectedShader->rebuildFromString(tempVertSource, tempFragSource);
        }

        // Edits the source file.
        if (ImGui::Button("Edit Source", ImVec2(buttonWidth, 0)))
        {
          editVert = true;
          this->vertEditor.SetText(this->selectedShader->getVertSource());

          editFrag = true;
          this->fragEditor.SetText(this->selectedShader->getFragSource());
        }

        // Saves the source to the shader file.
        ImGui::SameLine();
        if (ImGui::Button("Save Source", ImVec2(buttonWidth, 0)))
        {
          std::string tempVertSource = vertEditor.GetText();
          std::string tempFragSource = fragEditor.GetText();
          this->selectedShader->rebuildFromString(tempVertSource, tempFragSource);
          this->selectedShader->saveSourceToFiles();
        }

        // Display the shader information string.
        ImGui::Text("Shader information:");
        ImGui::Text(this->selectedShader->getInfoString().c_str());
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

    if (editVert)
    {
      std::string output = "";
      Styles::drawTextEditorWindow(this->vertEditor, "Vertex Source", editVert, output);
      if (output != "")
        this->selectedShader->setVertSource(output);
    }
    if (editFrag)
    {
      std::string output = "";
      Styles::drawTextEditorWindow(this->fragEditor, "Fragment Source", editFrag, output);
      if (output != "")
        this->selectedShader->setFragSource(output);
    }
  }

  void
  ShaderWindow::onUpdate(float dt)
  {

  }

  void
  ShaderWindow::onEvent(Event &event)
  {

  }
}
