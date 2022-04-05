#include "Assets/ModelAsset.h"

namespace Strontium
{
  ModelAsset::ModelAsset()
	: Asset(Type::Model)
  { }

  ModelAsset::~ModelAsset()
  { }
  
  void 
  ModelAsset::load(const std::filesystem::path &path)
  {
	this->path = path;
  }

  void ModelAsset::unload()
  {

  }
}