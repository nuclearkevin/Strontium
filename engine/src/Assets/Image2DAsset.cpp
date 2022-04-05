#include "Assets/Image2DAsset.h"

namespace Strontium
{
  Image2DAsset::Image2DAsset()
	: Asset(Type::Image2D)
  { }

  Image2DAsset::~Image2DAsset()
  { }
  
  void 
  Image2DAsset::Image2DAsset::load(const std::filesystem::path &path)
  {
	this->path = path;
  }

  void 
  Image2DAsset::Image2DAsset::unload()
  {

  }
}