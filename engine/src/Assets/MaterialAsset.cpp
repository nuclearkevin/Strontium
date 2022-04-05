#include "Assets/MaterialAsset.h"

namespace Strontium
{
  MaterialAsset::MaterialAsset()
    : Asset(Asset::Type::Material)
  { }
  
  MaterialAsset::~MaterialAsset() 
  { }
  
  void 
  MaterialAsset::load(const std::filesystem::path &path)
  {
    this->path = path;
  }

  void 
  MaterialAsset::unload() 
  {
    
  }
}