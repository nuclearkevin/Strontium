#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

#include "Graphics/Model.h"

namespace Strontium
{
  class ModelAsset final : public Asset
  {
  public:
  	ModelAsset();
  	~ModelAsset() override;
  
  	void load(const std::filesystem::path& path) override;
  	void unload() override;
  
  	Model* getModel() { return &model; }
  private:
  	Model model;
  };
}