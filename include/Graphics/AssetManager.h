#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

namespace SciRenderer
{
  template <class T>
  class AssetManager
  {
  public:
    AssetManager() = default;
    ~AssetManager() = default;

    // Create an asset without loading it from a file.
    template <typename ... Args>
    Shared<T> create(const std::string &name, Args ... args)
    {
      Shared<T> out = createShared<T>(std::forward<Args>(args)...);
      this->assetStorage.insert({ name, out });
      return out;
    }

    // Load an asset. MUST DECLARE TEMPLATE SPECIALIZATIONS IN THE HEADER FILE.
    Shared<T> loadFromFile(const std::string &filepath, const std::string &name);

    // Get the asset reference.
    Shared<T> getAsset(const std::string &name) { return this->assetStorage.at(name); }

    // Delete the asset.
    void deleteAsset(const std::string &name) { this->assetStorage.erase(name); }

  private:
    std::unordered_map<std::string, Shared<T>> assetStorage;
  };
}
