#include "GuiElements/ShaderWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Core/Logs.h"
#include "Graphics/Shaders.h"
#include "GuiElements/Styles.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

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
      ImGuiTreeNodeFlags flags = ((this->selectedShader == shaderCache->getAsset(name)) ? ImGuiTreeNodeFlags_Selected : 0);
      flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_Bullet;

      bool opened = ImGui::TreeNodeEx((void*) shaderCache->getAsset(name), flags, name.c_str());

      if (ImGui::IsItemClicked())
      {
        this->shaderName = name;
        this->selectedShader = shaderCache->getAsset(name);
      }

      if (this->selectedShader == shaderCache->getAsset(name) && opened)
      {
        ImGui::Separator();

        float buttonWidth = 140.0f;

        // Reloads the shader file.
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
