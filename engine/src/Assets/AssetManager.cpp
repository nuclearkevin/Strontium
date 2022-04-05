#include "Assets/AssetManager.h"

namespace Strontium
{
  AssetManager::AssetManager(const std::filesystem::path &assetRegistryPath)
	: registryPath(registryPath)
  {

  }

  AssetManager::~AssetManager()
  {

  }

  void 
  AssetManager::registerAsset(const std::filesystem::path& assetPath, const Asset::Handle& handle, 
                              Asset* asset)
  {
	asset->path = assetPath;
  }
	
  void 
  AssetManager::bakeAsset(const std::filesystem::path& assetPath, const Asset::Handle& handle, 
                          Asset::Type type)
  {

  }
}