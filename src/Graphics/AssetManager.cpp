#include "Graphics/AssetManager.h"

// Project includes.
#include "Graphics/Meshes.h"
#include "Graphics/Textures.h"

namespace SciRenderer
{
  template <typename T>
  Shared<T>
  AssetManager<T>::loadFromFile(const std::string &filepath, const std::string &name)
  {
    std::cout << "Behavior undefined!" << std::endl;
  }

  template <>
  Shared<Mesh>
  AssetManager<Mesh>::loadFromFile(const std::string &filepath, const std::string &name)
  {
    Shared<Mesh> loadable = createShared<Mesh>();
    loadable->loadOBJFile(filepath.c_str());
    loadable->generateVAO();
    this->assetStorage.insert({ name, loadable });
    return loadable;
  }

  template <>
  Shared<Texture2D>
  AssetManager<Texture2D>::loadFromFile(const std::string &filepath, const std::string &name)
  {
    Shared<Texture2D> loadable = createShared<Texture2D>();
    /*
    loadable->loadOBJFile(filepath.c_str());
    loadable->generateVAO();
    */
    this->assetStorage.insert({ name, loadable });
    return loadable;
  }

  template <>
  Shared<CubeMap>
  AssetManager<CubeMap>::loadFromFile(const std::string &filepath, const std::string &name)
  {
    Shared<CubeMap> loadable = createShared<CubeMap>();
    /*
    loadable->loadOBJFile(filepath.c_str());
    loadable->generateVAO();
    */
    this->assetStorage.insert({ name, loadable });
    return loadable;
  }
}
