#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Assets/Assets.h"
#include "Graphics/Textures.h"

namespace Strontium
{
  class Image2DAsset final : public Asset
  {
  public:
	Image2DAsset(ImageLoadOverride overload = ImageLoadOverride::None);
	~Image2DAsset() override;

	void load(const std::filesystem::path &path) override;
	void unload() override;

	Texture2D* getTexture() { return &tex; }
	ImageLoadOverride getOverride() { return this->overload; }
  private:
	Texture2D tex;

	ImageLoadOverride overload;
  };
}