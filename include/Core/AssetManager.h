#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Logs.h"
#include "Graphics/Model.h"

// STL includes. Mutex for thread safety.
#include <mutex>

namespace SciRenderer
{
  // A class to handle asset creation, destruction and file loading.
  template <class T>
  class AssetManager
  {
  public:
    ~AssetManager() = default;

    static std::mutex assetMutex;

    // Get an asset manager instance.
    static AssetManager<T>* getManager()
    {
      std::lock_guard<std::mutex> guard(assetMutex);

      if (instance == nullptr)
      {
        instance = new AssetManager<T>();
        return instance;
      }
      else
        return instance;
    }

    // Check to see if the map has an asset.
    bool hasAsset(const std::string &name)
    {
      return this->assetStorage.contains(name);
    }

    // Attach an asset to the manager.
    void attachAsset(const std::string &name, T* asset)
    {
      std::lock_guard<std::mutex> guard(assetMutex);

      if (!this->hasAsset(name))
      {
        this->assetNames.push_back(name);
        this->assetStorage.insert({ name, Unique<T>(asset) });
      }
    }

    // Get the asset reference.
    T* getAsset(const std::string &name)
    {
      if (this->hasAsset(name))
        return this->assetStorage.at(name).get();
      else
        return nullptr;
    }

    // Delete the asset.
    void deleteAsset(const std::string &name)
    {
      std::lock_guard<std::mutex> guard(assetMutex);

      this->assetStorage.erase(name);

      auto loc = std::find(this->assetNames.begin(), this->assetNames.end(), name);
      if (loc != this->assetNames.end())
        this->assetNames.erase(loc);
    }

    // Get a reference to the asset name storage.
    std::vector<std::string>& getStorage() { return this->assetNames; }
  private:
    AssetManager() = default;

    static AssetManager<T>* instance;

    std::unordered_map<std::string, Unique<T>> assetStorage;
    std::vector<std::string> assetNames;
  };

  template <typename T>
  AssetManager<T>* AssetManager<T>::instance = nullptr;

  template <typename T>
  std::mutex AssetManager<T>::assetMutex;
}
