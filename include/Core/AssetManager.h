#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Logs.h"
#include "Graphics/Model.h"

namespace SciRenderer
{
  // A class to handle asset creation, destruction and file loading.
  template <class T>
  class AssetManager
  {
  public:
    ~AssetManager() = default;

    // Get an asset manager instance.
    static AssetManager<T>* getManager()
    {
      if (instance == nullptr)
      {
        instance = new AssetManager<T>();
        return instance;
      }
      else
      {
        return instance;
      }
    }

    // Check to see if the map has an asset.
    bool hasAsset(const std::string &name) { return this->assetStorage.contains(name); }

    // Attach an asset to the manager.
    void attachAsset(const std::string &name, T* asset)
    {
      if (!this->hasAsset(name))
      {
        this->assetNames.push_back(name);
        this->assetStorage.insert({ name, Unique<T>(asset) });
      }
    }

    // Load an asset. MUST DECLARE TEMPLATE SPECIALIZATIONS IN THE HEADER FILE.
    T* loadAssetFile(const std::string &filepath, const std::string &name);

    // Get the asset reference.
    T* getAsset(const std::string &name) { return this->assetStorage.at(name).get(); }

    // Delete the asset.
    void deleteAsset(const std::string &name)
    {
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

  //----------------------------------------------------------------------------
  // Template specializations for specific asset types go here.
  //----------------------------------------------------------------------------
  template class AssetManager<Model>;

  template <typename T>
  T*
  AssetManager<T>::loadAssetFile(const std::string &filepath, const std::string &name)
  {
    std::cout << "Behavior undefined!" << std::endl;
    return nullptr;
  }

  template <>
  Model*
  AssetManager<Model>::loadAssetFile(const std::string &filepath, const std::string &name)
  {
    if (this->hasAsset(name))
      return this->assetStorage.at(name).get();

    Model* loadable = new Model();
    loadable->loadModel(filepath);
    this->assetStorage.insert({ name, Unique<Model>(loadable) });
    this->assetNames.push_back(name);

    Logger* logs = Logger::getInstance();
    logs->logMessage(LogMessage("Asset loaded at the path " + filepath +
                                " with the name " + name + ".", true, false, true));

    return loadable;
  }
}
