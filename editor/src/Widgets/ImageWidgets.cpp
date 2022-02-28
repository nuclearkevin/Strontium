#include "Widgets/ImageWidgets.h"

namespace Strontium
{
  TextureWidget::TextureWidget(const std::string & widgetName)
    : texCubemapTex2D(ShaderCache::getShader("cubemap_tex_2D"))
    , texCubemapArrayTex2D(ShaderCache::getShader("cubemap_array_tex_2D"))
    , tex2DArrayTex2D(ShaderCache::getShader("tex_2D_array_tex_2D"))
    , widgetName(widgetName)
    , lod(0.0f)
    , slice(0)
  {
    auto params = Texture2D::getFloatColourParams();

    this->buffer.setSize(128, 128);
    this->buffer.setParams(params);
    this->buffer.initNullTexture();
  }

  TextureWidget::~TextureWidget()
  { }
  
  void 
  TextureWidget::cubemapImage(CubeMap &texture, const ImVec2 &size, 
  				              bool lodSlider, const ImVec2 &uv1, 
  				              const ImVec2 &uv2)
  {
    // Resize the buffer if necessary.
    if (static_cast<int>(size.x) != this->buffer.getWidth() || 
        static_cast<int>(size.y) != this->buffer.getHeight())
    {
      this->buffer.setSize(size.x, size.y);
      this->buffer.initNullTexture();
    }

    ImGui::PushID(this->widgetName.c_str());
    ImGui::Text(this->widgetName.c_str());
    // Lod slider behavior.
    if (lodSlider)
    {
      // Compute the max lod of the texture.
      int maxDim = glm::max(this->buffer.getWidth(),
	  					  this->buffer.getHeight());
      float maxLod = glm::max(glm::ceil(glm::log2(static_cast<float>(maxDim))) - 1.0f, 0.0f);
      
      ImGui::SliderFloat("Cubemap LOD", &this->lod, 0.0f, maxLod);
    }

    texture.bind(0);
    this->texCubemapTex2D->addUniformFloat("u_lod", this->lod);
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->buffer.getWidth()) / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->buffer.getHeight()) / 8.0f));
    this->buffer.bindAsImage(0, 0, ImageAccessPolicy::Write);
    this->texCubemapTex2D->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    ImGui::Image(reinterpret_cast<ImTextureID>(buffer.getID()),
                 size, uv1, uv2);

    ImGui::PopID();
  }

  void 
  TextureWidget::cubemapArrayImage(CubeMapArrayTexture &texture, const ImVec2 &size,
					               bool lodSlider, const ImVec2 &uv1, const ImVec2 &uv2)
  {
    // Resize the buffer if necessary.
    if (static_cast<int>(size.x) != this->buffer.getWidth() || 
        static_cast<int>(size.y) != this->buffer.getHeight())
    {
      this->buffer.setSize(size.x, size.y);
      this->buffer.initNullTexture();
    }

    ImGui::PushID(this->widgetName.c_str());
    ImGui::Text(this->widgetName.c_str());
    // Array slice slider behavior.
    int maxSlice = static_cast<int>(texture.getLayers()) - 1;
    ImGui::SliderInt("Array Texture Slice", &this->slice, 0, maxSlice);

    // Lod slider behavior.
    if (lodSlider)
    {
      // Compute the max lod of the texture.
      int maxDim = glm::max(this->buffer.getWidth(),
	  					  this->buffer.getHeight());
      float maxLod = glm::max(glm::ceil(glm::log2(static_cast<float>(maxDim))) - 1.0f, 0.0f);
      
      ImGui::SliderFloat("Cubemap LOD", &this->lod, 0.0f, maxLod);
    }

    texture.bind(0);
    this->texCubemapArrayTex2D->addUniformFloat("u_lod", this->lod);
    this->texCubemapArrayTex2D->addUniformFloat("u_slice", static_cast<float>(this->slice));
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->buffer.getWidth()) / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->buffer.getHeight()) / 8.0f));
    this->buffer.bindAsImage(0, 0, ImageAccessPolicy::Write);
    this->texCubemapArrayTex2D->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    ImGui::PopID();
    ImGui::Image(reinterpret_cast<ImTextureID>(buffer.getID()),
                 size, uv1, uv2);
  }

  void 
  TextureWidget::arrayImage(Texture2DArray &texture, const ImVec2 &size, 
  				            bool lodSlider, const ImVec2 &uv1, 
  				            const ImVec2 &uv2)
  {
    // Resize the buffer if necessary.
    if (static_cast<int>(size.x) != this->buffer.getWidth() || 
        static_cast<int>(size.y) != this->buffer.getHeight())
    {
      this->buffer.setSize(size.x, size.y);
      this->buffer.initNullTexture();
    }

    ImGui::PushID(this->widgetName.c_str());
    ImGui::Text(this->widgetName.c_str());
    // Array slice slider behavior.
    int maxSlice = static_cast<int>(texture.getLayers()) - 1;
    ImGui::SliderInt("Array Texture Slice", &this->slice, 0, maxSlice);

    // Lod slider behavior.
    if (lodSlider)
    {
      // Compute the max lod of the texture.
      int maxDim = glm::max(this->buffer.getWidth(),
	  					  this->buffer.getHeight());
      float maxLod = glm::max(glm::ceil(glm::log2(static_cast<float>(maxDim))) - 1.0f, 0.0f);
      
      ImGui::SliderFloat("Array Texture LOD", &this->lod, 0.0f, maxLod);
    }

    texture.bind(0);
    this->tex2DArrayTex2D->addUniformFloat("u_lod", this->lod);
    this->tex2DArrayTex2D->addUniformFloat("u_slice", static_cast<float>(this->slice));
    uint iWidth = static_cast<uint>(glm::ceil(static_cast<float>(this->buffer.getWidth()) / 8.0f));
    uint iHeight = static_cast<uint>(glm::ceil(static_cast<float>(this->buffer.getHeight()) / 8.0f));
    this->buffer.bindAsImage(0, 0, ImageAccessPolicy::Write);
    this->tex2DArrayTex2D->launchCompute(iWidth, iHeight, 1);
    Shader::memoryBarrier(MemoryBarrierType::ShaderImageAccess);

    ImGui::PopID();
    ImGui::Image(reinterpret_cast<ImTextureID>(buffer.getID()),
                 size, uv1, uv2);
  }
}