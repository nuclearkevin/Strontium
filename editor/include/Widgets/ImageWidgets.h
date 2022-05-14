#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Graphics/Shaders.h"
#include "Graphics/Textures.h"

// ImGui includes.
#include "imgui/imgui.h"

namespace Strontium
{
  class TextureWidget
  {
  public:
	TextureWidget(const std::string &widgetName);
	~TextureWidget();

	void texture2DImageLod(Texture2D &texture, const ImVec2& size,
						   const ImVec2 &uv1 = ImVec2(0, 1), 
						   const ImVec2 &uv2 = ImVec2(1, 0));
	void cubemapImage(CubeMap &texture, const ImVec2 &size, 
					  bool lodSlider = false,
					  const ImVec2 &uv1 = ImVec2(0, 1), 
					  const ImVec2 &uv2 = ImVec2(1, 0));
	void cubemapArrayImage(CubeMapArrayTexture &texture, const ImVec2 &size,
					       bool lodSlider = false,
					       const ImVec2 &uv1 = ImVec2(0, 1), 
					       const ImVec2 &uv2 = ImVec2(1, 0));
	void arrayImage(Texture2DArray &texture, const ImVec2 &size, 
					bool lodSlider = false,
					const ImVec2 &uv1 = ImVec2(0, 1), 
					const ImVec2 &uv2 = ImVec2(1, 0));
	void texture3DImage(Texture3D &texture, const ImVec2& size, bool lodSlider = false,
		                const ImVec2& uv1 = ImVec2(0, 1),
		                const ImVec2& uv2 = ImVec2(1, 0));

  private:
	Texture2D buffer;

	Shader* tex2DLod;
	Shader* texCubemapTex2D;
	Shader* texCubemapArrayTex2D;
	Shader* tex2DArrayTex2D;
	Shader* tex3DTex2D;

	std::string widgetName;

	float lod;
	int slice;
  };
}