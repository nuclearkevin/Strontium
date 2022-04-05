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
	this->model.load(path);
  }

  void ModelAsset::unload()
  {

  }
}