#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"

// ImGui includes.
#include "imgui/imgui.h"

namespace Strontium
{
  class LightWidget
  {
  public:
	LightWidget(const std::string &widgetName);
	~LightWidget();

	void dirLightManip(glm::mat4 &transform, const ImVec2 &size,
		               const ImVec2& uv1 = ImVec2(0, 1),
		               const ImVec2& uv2 = ImVec2(1, 0));
  private:
	Texture2D buffer;

	Shader* dirLightWidget;

	std::string widgetName;
  };
}