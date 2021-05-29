#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace SciRenderer
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

    void drawVec3Controls(const std::string &label, glm::vec3 resetValue, glm::vec3& param);
    void drawVec2Controls(const std::string &label, glm::vec2 resetValue, glm::vec2& param);
    void drawFloatControl(const std::string &label, GLfloat resetValue, GLfloat& param);

    ImVec4 colourFromHex(const std::string &hex);
  }
}
