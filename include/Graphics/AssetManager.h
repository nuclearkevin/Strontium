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

    // Load an asset.
    Shared<T> loadFromFile(const std::string &filepath, const std::string &name);

    // Get the asset reference.
    Shared<T> getAsset(const std::string &name);

    // Delete the asset.
    void deleteAsset(const std::string &name);

  protected:
    std::unordered_map<std::string, Shared<T>> assetStorage;
  };
}
