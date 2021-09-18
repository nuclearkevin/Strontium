#include "GuiElements/Styles.h"

#include <sstream>

namespace Strontium
{
  namespace Styles
  {
    void
    setDarkModeTheme()
    {
      ImGuiStyle& style = ImGui::GetStyle();

    	style.WindowPadding = { 2, 2 };
    	style.WindowBorderSize = 0;

    	style.FrameRounding = 2;
    	style.WindowRounding = 2;
    	style.FrameBorderSize = 1;
    	style.ScrollbarRounding = 6;
    	style.IndentSpacing = 16;
    	style.GrabRounding = 2;
    	style.ItemInnerSpacing = { 6, 4 };
    	style.TabRounding = 2;
    	style.WindowMenuButtonPosition = ImGuiDir_Left;

    	setColour(ImGuiCol_MenuBarBg, "#181816");
    	setColour(ImGuiCol_Text, "#B9B9B9");
    	setColour(ImGuiCol_TextSelectedBg, "##5DC5054B");

    	setColour(ImGuiCol_WindowBg, "#222222");
    	setColour(ImGuiCol_ScrollbarBg, "#222222");
    	setColour(ImGuiCol_TitleBgActive, "#181816");

    	setColour(ImGuiCol_Separator, "#181816");
    	setColour(ImGuiCol_SeparatorHovered, "#5DC50570");
    	setColour(ImGuiCol_SeparatorActive, "#53B305");

    	setColour(ImGuiCol_Border, "#303030");
    	setColour(ImGuiCol_CheckMark, "#5DC505");
    	setColour(ImGuiCol_NavHighlight, "#5DC50590");

    	setColour(ImGuiCol_FrameBg, "#0D0D0B");
    	setColour(ImGuiCol_FrameBgHovered, "#0B0B09");
    	setColour(ImGuiCol_FrameBgActive, "#0B0B09");

    	setColour(ImGuiCol_Header, "#323234");
    	setColour(ImGuiCol_HeaderHovered, "#2e2e30");

    	setColour(ImGuiCol_Tab, "#181816");
    	setColour(ImGuiCol_TabActive, "#222222");
    	setColour(ImGuiCol_TabUnfocused, "#161616");
    	setColour(ImGuiCol_TabUnfocusedActive, "#222222");

    	setColour(ImGuiCol_DockingPreview, "#5DC5052A");

    	setColour(ImGuiCol_Button, "#222222");
    	setColour(ImGuiCol_ButtonHovered, "#181818");
    	setColour(ImGuiCol_ButtonActive, "#222222");

    	setColour(ImGuiCol_SliderGrab, "#303030");

    	setColour(ImGuiCol_ResizeGrip, "#181816");
    	setColour(ImGuiCol_ResizeGripHovered, "#5DC50570");
    	setColour(ImGuiCol_ResizeGripActive, "#53B305");
    }

    void
    setDefaultTheme()
    {
      ImGuiStyle& style = ImGui::GetStyle();

    	style.WindowPadding = ImVec2(4, 4);
    	style.FramePadding = ImVec2(6, 2);

    	style.WindowTitleAlign = ImVec2(0, 0.5);
    	style.WindowMenuButtonPosition = ImGuiDir_Left;

    	style.IndentSpacing = 8;
    	style.ScrollbarSize = 16;

    	style.FrameRounding = 2;
    	style.ScrollbarRounding = 2;
    	style.WindowRounding = 0;
    	style.GrabRounding = 0;
    	style.ScrollbarRounding = 0;
    	style.TabRounding = 2;

    	setColour(ImGuiCol_WindowBg, "#181818");
    	setColour(ImGuiCol_DockingPreview, "#202020");
    	setColour(ImGuiCol_MenuBarBg, "#141414");
    	setColour(ImGuiCol_NavHighlight, "#53B305");

    	setColour(ImGuiCol_Separator, "#262626");
    	setColour(ImGuiCol_SeparatorHovered, "#212121");
    	setColour(ImGuiCol_SeparatorActive, "#53B305");

    	setColour(ImGuiCol_TitleBg, "#161616");
    	setColour(ImGuiCol_TitleBgActive, "#242424");
    	setColour(ImGuiCol_Header, "#242424");
    	setColour(ImGuiCol_HeaderHovered, "#202020");
    	setColour(ImGuiCol_HeaderActive, "#2E6303");

    	setColour(ImGuiCol_Tab, "#262626");
    	setColour(ImGuiCol_TabUnfocusedActive, "#262626");
    	setColour(ImGuiCol_TabActive, "#202020");
    	setColour(ImGuiCol_TabUnfocused, "#242424");
    	setColour(ImGuiCol_TabHovered, "#3E6303");

    	setColour(ImGuiCol_FrameBg, "#242424");
    	setColour(ImGuiCol_FrameBgHovered, "#222222");
    	setColour(ImGuiCol_FrameBgActive, "#202020");
    	setColour(ImGuiCol_ResizeGrip, "#242424");
    	setColour(ImGuiCol_ResizeGripHovered, "#2E6303");
    	setColour(ImGuiCol_ResizeGripActive, "#53B305");

    	setColour(ImGuiCol_Text, "#B9B9B9");
    	setColour(ImGuiCol_TextDisabled, "#A9A9A9");

    	setColour(ImGuiCol_PlotLines, "#5DC505");
    	setColour(ImGuiCol_Border, "#131313");

    	setColour(ImGuiCol_CheckMark, "#53B305");
    	setColour(ImGuiCol_SliderGrab, "#353535");
    	setColour(ImGuiCol_SliderGrabActive, "#53B305");

    	setColour(ImGuiCol_Button, "#262626");
    	setColour(ImGuiCol_ButtonHovered, "#222222");
    	setColour(ImGuiCol_ButtonActive, "#2E6303");

    	setColour(ImGuiCol_ScrollbarGrab, "#2E6303");
    	setColour(ImGuiCol_ScrollbarGrabHovered, "#4A9F04");
    	setColour(ImGuiCol_ScrollbarGrabActive, "#418B04");
    }

    void
    drawVec3Controls(const std::string &label, glm::vec3 resetValue,
                     glm::vec3& param, GLfloat offset, GLfloat speed,
                     GLfloat min, GLfloat max)
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
      ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
      setButtonColour(ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
      if (ImGui::Button((std::string("Reset##") + label).c_str()))
        param = resetValue;
      ImGui::PopItemWidth();
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      ImGui::DragFloat((std::string("##x") + label).c_str(), &param.x, speed,
                       min, max, "%.2f");
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::DragFloat((std::string("##y") + label).c_str(), &param.y, speed,
                       min, max, "%.2f");
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::DragFloat((std::string("##z") + label).c_str(), &param.z, speed,
                       min, max, "%.2f");
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::Text(label.c_str());
      ImGui::PopStyleVar();
    }

    void
    drawVec2Controls(const std::string &label, glm::vec2 resetValue,
                     glm::vec2& param, GLfloat offset, GLfloat speed,
                     GLfloat min, GLfloat max)
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
      ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth() * 4.5 / 4.0 - offset);
      setButtonColour(ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f },
                      ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f },
                      ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
      if (ImGui::Button((std::string("Reset##") + label).c_str()))
        param = resetValue;
      ImGui::PopItemWidth();
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      ImGui::DragFloat((std::string("##x") + label).c_str(), &param.x, speed,
                       min, max, "%.2f");
      ImGui::PopItemWidth();
      ImGui::SameLine();
      ImGui::DragFloat((std::string("##y") + label).c_str(), &param.y, speed,
                       min, max, "%.2f");
      ImGui::PopItemWidth();
      ImGui::SameLine();
      ImGui::Text(label.c_str());
      ImGui::PopStyleVar();
    }

    void
    drawFloatControl(const std::string &label, GLfloat resetValue,
                     GLfloat& param, GLfloat offset, GLfloat speed,
                     GLfloat min, GLfloat max)
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
      setButtonColour(ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f },
                      ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f },
                      ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
      if (ImGui::Button((std::string("Reset##") + label).c_str()))
        param = resetValue;
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      ImGui::PushItemWidth(ImGui::CalcItemWidth() * 3.0 / 4.0 - offset);
      ImGui::DragFloat((std::string("##x") + label).c_str(), &param, speed, min,
                       max, "%.2f");
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::Text(label.c_str());
      ImGui::PopStyleVar();
    }

    void
    setColour(ImGuiCol_ param, const std::string &hex)
    {
      auto colour = colourFromHex(hex);

      ImGuiStyle& style = ImGui::GetStyle();
      style.Colors[param] = colour;
    }

    void
    setButtonColour(const std::string &hexDefault, const std::string &hexHovered,
                    const std::string &hexActive)
    {
      ImGui::PushStyleColor(ImGuiCol_Button, colourFromHex(hexDefault));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colourFromHex(hexHovered));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, colourFromHex(hexActive));
    }

    void setButtonColour(ImVec4 cDefault, ImVec4 cHovered, ImVec4 cActive)
    {
      ImGui::PushStyleColor(ImGuiCol_Button, cDefault);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, cHovered);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, cActive);
    }

    ImVec4
    colourFromHex(const std::string &hex)
    {
      if (hex.substr(0, 1) != "#")
        return ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

      int r, g, b, a = 255;
      r = std::strtoul(hex.substr(1, 2).c_str(), 0, 16);
      g = std::strtoul(hex.substr(3, 2).c_str(), 0, 16);
      b = std::strtoul(hex.substr(5, 2).c_str(), 0, 16);

      if (hex.length() == 9)
        a = std::strtoul(hex.substr(7, 2).c_str(), 0, 16);

      return ImVec4(((GLfloat) r) / 255.0f, ((GLfloat) g) / 255.0f,
                    ((GLfloat) b) / 255.0f, ((GLfloat) a) / 255.0f);
    }

    std::string
    colourToHex(const ImVec4 &colour)
    {
      int r = (int) (colour.x * 255.0f);
      int g = (int) (colour.y * 255.0f);
      int b = (int) (colour.z * 255.0f);
      int a = (int) (colour.w * 255.0f);

      unsigned long out = ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);
      std::stringstream stream;
      stream << "#" << std::hex << out;
      return std::string(stream.str()).substr(0, 9);
    }

    std::string
    colourToHex(const glm::vec4 &colour)
    {
      int r = (int) (colour.r * 255.0f);
      int g = (int) (colour.g * 255.0f);
      int b = (int) (colour.b * 255.0f);
      int a = (int) (colour.a * 255.0f);

      unsigned long out = ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);
      std::stringstream stream;
      stream << "#" << std::hex << out;
      return std::string(stream.str()).substr(0, 9);
    }

    void
    drawTextEditorWindow(TextEditor &editor, const std::string &name, bool &isOpen, std::string &outString)
    {
      auto flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar;
      auto cpos = editor.GetCursorPosition();

      ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
      ImGui::Begin((std::string("Text Editor-") + name).c_str(), &isOpen, flags);
      if (ImGui::BeginMenuBar())
      {
        if (ImGui::BeginMenu("File"))
        {
          if (ImGui::MenuItem("Save", "Ctrl-S"))
          {
            outString = editor.GetText();
          }

          if (ImGui::MenuItem("Quit", "Alt-F4"))
          {
            isOpen = false;
          }

          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
          bool ro = editor.IsReadOnly();
          if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
          {
            editor.SetReadOnly(ro);
          }
          ImGui::Separator();

          if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
          {
            editor.Undo();
          }

          if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
          {
            editor.Redo();
          }
          ImGui::Separator();

          if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
          {
            editor.Copy();
          }

          if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
          {
            editor.Cut();
          }

          if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
          {
            editor.Delete();
          }

          if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
          {
            editor.Paste();
          }
          ImGui::Separator();

          if (ImGui::MenuItem("Select all", nullptr, nullptr))
          {
            editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));
          }

          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
  			{
          if (ImGui::MenuItem("Dark palette"))
          {
            editor.SetPalette(TextEditor::GetDarkPalette());
          }

          if (ImGui::MenuItem("Light palette"))
          {
            editor.SetPalette(TextEditor::GetLightPalette());
          }

          if (ImGui::MenuItem("Retro blue palette"))
          {
            editor.SetPalette(TextEditor::GetRetroBluePalette());
          }

          ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
      }

      ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s",
                  cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
                  editor.IsOverwrite() ? "Ovr" : "Ins", editor.CanUndo() ? "*" : " ",
                  editor.GetLanguageDefinition().mName.c_str(), name.c_str());

      editor.Render((std::string("##") + name).c_str());

      ImGui::End();
    }
  }
}

namespace ImGui
{
  // A loading spinner from https://github.com/ocornut/imgui/issues/1901.
  // Many thanks to Zelimir Fedoran for creating this and allowing for its
  // public use!

  bool Spinner(const char* label, float radius, int thickness, const ImU32& color)
  {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size((radius )*2, (radius + style.FramePadding.y)*2);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    // Render
    window->DrawList->PathClear();

    int num_segments = 30;
    int start = abs(ImSin(g.Time*1.8f)*(num_segments-5));

    const float a_min = IM_PI*2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI*2.0f * ((float)num_segments-3) / (float)num_segments;

    const ImVec2 centre = ImVec2(pos.x+radius, pos.y+radius+style.FramePadding.y);

    for (int i = 0; i < num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a+g.Time*8) * radius,
                                            centre.y + ImSin(a+g.Time*8) * radius));
    }

    window->DrawList->PathStroke(color, false, thickness);

    return true;
  }
}
