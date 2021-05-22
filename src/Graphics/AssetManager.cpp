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
    Shared<Texture2D> loadable = Textures::loadTexture2D(filepath);
    this->assetStorage.insert({ name, loadable });
    return loadable;
  }

  // Can only do .jpg files so far. TODO: Make this better.
  template <>
  Shared<CubeMap>
  AssetManager<CubeMap>::loadFromFile(const std::string &filepath, const std::string &name)
  {
    std::vector<std::string> sFaces = { "/posX", "/negX", "/posY", "/negY", "/posZ", "/negZ" };
    std::vector<std::string> cFaces;
    std::string temp;

    // Load each face of a cubemap.
    for (GLuint i = 0; i < 6; i++)
    {
      temp = filepath;
      temp += sFaces[i] + ".jpg";
      cFaces.push_back(temp);
    }
    Shared<CubeMap> loadable = Textures::loadTextureCubeMap(cFaces);

    this->assetStorage.insert({ name, loadable });
    return loadable;
  }
}
