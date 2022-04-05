#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

#include "Assets/Assets.h"

#include "Graphics/Material.h"

namespace Strontium
{
  class MaterialAsset final : public Asset
  {
  public:
	MaterialAsset();

	~MaterialAsset() override;

	void load(const std::filesystem::path &path) override;
	void unload() override;

	Material* getMaterial() { return &material; }
  private:
	Material material;
  };
}