#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// Font awesome includes.
#include "GuiElements/IconsFontAwesome4.h"

// ImGui text editor include.
#include "imguitexteditor/TextEditor.h"

namespace Strontium
{
  namespace Styles
  {
    // Set style colours. Thanks to ToniPlays#2749 from TheCherno's Discord
    // server for the excellent styles!
    void setDefaultTheme();
    void setDarkModeTheme();

    void setColour(ImGuiCol_ param, const std::string &hex);

    void setButtonColour(const std::string &hexDefault, const std::string &hexHovered,
                         const std::string &hexActive);

    void setButtonColour(ImVec4 cDefault, ImVec4 cHovered, ImVec4 cActive);

    void drawVec3Controls(const std::string &label, glm::vec3 resetValue, glm::vec3& param,
                          GLfloat offset = 0.0f, GLfloat speed = 0.1f,
                          GLfloat min = 0.0f, GLfloat max = 0.0f);
    void drawVec2Controls(const std::string &label, glm::vec2 resetValue, glm::vec2& param,
                          GLfloat offset = 0.0f, GLfloat speed = 0.1f,
                          GLfloat min = 0.0f, GLfloat max = 0.0f);
    void drawFloatControl(const std::string &label, GLfloat resetValue, GLfloat& param,
                          GLfloat offset = 0.0f, GLfloat speed = 0.1f,
                          GLfloat min = 0.0f, GLfloat max = 0.0f);

    ImVec4 colourFromHex(const std::string &hex);
    std::string colourToHex(const ImVec4 &colour);
    std::string colourToHex(const glm::vec4 &colour);

    void drawTextEditorWindow(TextEditor &editor, const std::string &name,
                              bool &isOpen, std::string &outString);
  }
}

// ImGUi objects.
namespace ImGui
{
  // A loading spinner from https://github.com/ocornut/imgui/issues/1901.
  // Many thanks to Zelimir Fedoran for creating this and allowing for its
  // public use!
  bool Spinner(const char* label, float radius, int thickness, const ImU32& color);
}
