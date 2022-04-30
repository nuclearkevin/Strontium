#include "Utils/AsyncAssetLoading.h"

// Project includes.
#include "Core/Events.h"
#include "Core/JobSystem.h"
#include "Core/Application.h"
#include "Core/DataStructures/ThreadSafeQueue.h"

#include "Assets/Image2DAsset.h"
#include "Assets/ModelAsset.h"

#include "Graphics/Material.h"

#include "Scenes/Entity.h"
#include "Scenes/Components.h"

// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

// STL includes.
#include <filesystem>

namespace Strontium
{
  namespace AsyncLoading
  {
    //--------------------------------------------------------------------------
    // Models, materials and meshes.
    //--------------------------------------------------------------------------
    ThreadSafeQueue<std::tuple<ModelAsset*, Asset::Handle, std::filesystem::path, Scene*, uint>> asyncModelQueue;

    void
    bulkGenerateMaterials()
    {
      // Filepaths of the textures that need to be loaded.
      std::vector<std::pair<std::string, ImageLoadOverride>> texturesToLoad;
      // Reserve for the worst case scenario.
      texturesToLoad.reserve(asyncModelQueue.size() * 6);

      auto& assetCache = Application::getInstance()->getAssetCache();

      while (!asyncModelQueue.empty())
      {
        auto [result, handle, path, activeScene, entityID] = asyncModelQueue.front();
        Entity entity(static_cast<entt::entity>(entityID), activeScene);

        if (!assetCache.has<ModelAsset>(handle) && result)
          assetCache.attach<ModelAsset>(result, path, handle);

        auto modelAsset = assetCache.get<ModelAsset>(handle);

        if (entity)
        {
          if (entity.hasComponent<RenderableComponent>())
          {
            auto& rComponent = entity.getComponent<RenderableComponent>();
            auto& materials = rComponent.materials;
            auto& submeshes = modelAsset->getModel()->getSubmeshes();

            if (rComponent.animationHandle != "")
            {
              for (auto& animation : modelAsset->getModel()->getAnimations())
              {
                if (animation.getName() == rComponent.animationHandle)
                {
                  rComponent.animator.setAnimation(&animation, rComponent.meshName);
                  rComponent.animationHandle = "";
                  rComponent.animator.startAnimation();
                  break;
                }
              }
            }

            if (materials.getNumStored() == 0)
            {
              for (auto& submesh : submeshes)
              {
                std::string submeshName = submesh.getName();
                auto submeshTexturePaths = submesh.getMaterialInfo();

                materials.attachMesh(submeshName, Material::Type::PBR);
                auto submeshMaterial = materials.getMaterial(submeshName);

                std::string texName;
                std::filesystem::path currentTexturePath;
                if (submeshTexturePaths.albedoTexturePath != "")
                {
                  currentTexturePath = std::filesystem::path(submeshTexturePaths.albedoTexturePath);
                  submeshMaterial->set(glm::vec3(submeshTexturePaths.albedoTint), "uAlbedo");
                  texName = currentTexturePath.filename().string();
                  submeshMaterial->attachSampler2D("albedoMap", texName);

                  bool shouldLoad = std::find_if(texturesToLoad.begin(),
                                                 texturesToLoad.end(), 
                                                 [currentTexturePath](const std::pair<std::string, ImageLoadOverride> &pair)
                  {
                    return currentTexturePath.string() == pair.first;
                  }) == texturesToLoad.end();

                  if (!assetCache.has<Image2DAsset>(texName) && shouldLoad)
                    texturesToLoad.emplace_back(currentTexturePath.string(), ImageLoadOverride::None);
                }

                if (submeshTexturePaths.roughnessTexturePath != "" && !submeshTexturePaths.hasCombinedMR)
                {
                  currentTexturePath = std::filesystem::path(submeshTexturePaths.roughnessTexturePath);
                  submeshMaterial->set(submeshTexturePaths.roughnessScale, "uRoughness");
                  texName = currentTexturePath.filename().string();
                  submeshMaterial->attachSampler2D("roughnessMap", texName);

                  bool shouldLoad = std::find_if(texturesToLoad.begin(),
                                                 texturesToLoad.end(), 
                                                 [currentTexturePath](const std::pair<std::string, ImageLoadOverride> &pair)
                  {
                    return currentTexturePath.string() == pair.first;
                  }) == texturesToLoad.end();

                  if (!assetCache.has<Image2DAsset>(texName) && shouldLoad)
                    texturesToLoad.emplace_back(currentTexturePath.string(), ImageLoadOverride::None);
                }

                if (submeshTexturePaths.metallicTexturePath != "" && !submeshTexturePaths.hasCombinedMR)
                {
                  currentTexturePath = std::filesystem::path(submeshTexturePaths.metallicTexturePath);
                  submeshMaterial->set(submeshTexturePaths.metallicScale, "uMetallic");
                  texName = currentTexturePath.filename().string();
                  submeshMaterial->attachSampler2D("metallicMap", texName);

                  bool shouldLoad = std::find_if(texturesToLoad.begin(),
                                                 texturesToLoad.end(), 
                                                 [currentTexturePath](const std::pair<std::string, ImageLoadOverride> &pair)
                  {
                    return currentTexturePath.string() == pair.first;
                  }) == texturesToLoad.end();

                  if (!assetCache.has<Image2DAsset>(texName) && shouldLoad)
                    texturesToLoad.emplace_back(currentTexturePath.string(), ImageLoadOverride::None);
                }

                if (submeshTexturePaths.aoTexturePath != "")
                {
                  currentTexturePath = std::filesystem::path(submeshTexturePaths.aoTexturePath);
                  submeshMaterial->set(submeshTexturePaths.aoScale, "uAO");
                  texName = currentTexturePath.filename().string();
                  submeshMaterial->attachSampler2D("aOcclusionMap", texName);

                  bool shouldLoad = std::find_if(texturesToLoad.begin(),
                                                 texturesToLoad.end(), 
                                                 [currentTexturePath](const std::pair<std::string, ImageLoadOverride> &pair)
                  {
                    return currentTexturePath.string() == pair.first;
                  }) == texturesToLoad.end();

                  if (!assetCache.has<Image2DAsset>(texName) && shouldLoad)
                    texturesToLoad.emplace_back(currentTexturePath.string(), ImageLoadOverride::None);
                }

                if (submeshTexturePaths.specularTexturePath != "")
                {
                  currentTexturePath = std::filesystem::path(submeshTexturePaths.specularTexturePath);
                  submeshMaterial->set(0.04f, "uF0");
                  texName = currentTexturePath.filename().string();
                  submeshMaterial->attachSampler2D("specF0Map", texName);

                  bool shouldLoad = std::find_if(texturesToLoad.begin(),
                                                 texturesToLoad.end(), 
                                                 [currentTexturePath](const std::pair<std::string, ImageLoadOverride> &pair)
                  {
                    return currentTexturePath.string() == pair.first;
                  }) == texturesToLoad.end();

                  if (!assetCache.has<Image2DAsset>(texName) && shouldLoad)
                    texturesToLoad.emplace_back(currentTexturePath.string(), ImageLoadOverride::None);
                }

                if (submeshTexturePaths.normalTexturePath != "")
                {
                  currentTexturePath = std::filesystem::path(submeshTexturePaths.normalTexturePath);
                  texName = currentTexturePath.filename().string();
                  submeshMaterial->attachSampler2D("normalMap", texName);

                  bool shouldLoad = std::find_if(texturesToLoad.begin(),
                                                 texturesToLoad.end(), 
                                                 [currentTexturePath](const std::pair<std::string, ImageLoadOverride> &pair)
                  {
                    return currentTexturePath.string() == pair.first;
                  }) == texturesToLoad.end();

                  if (!assetCache.has<Image2DAsset>(texName) && shouldLoad)
                    texturesToLoad.emplace_back(currentTexturePath.string(), ImageLoadOverride::None);
                }

                // Handle GLTF roughness and metalness parameters being combined in a single texture.
                if (submeshTexturePaths.hasCombinedMR)
                {
                  // Metallic and roughness texture paths are the same.
                  currentTexturePath = std::filesystem::path(submeshTexturePaths.metallicTexturePath);
                  texName = currentTexturePath.filename().string();
                  submeshMaterial->set(submeshTexturePaths.metallicScale, "uMetallic");
                  submeshMaterial->attachSampler2D("metallicMap", texName + "_m");
                  submeshMaterial->set(submeshTexturePaths.roughnessScale, "uRoughness");
                  submeshMaterial->attachSampler2D("roughnessMap", texName + "_r");

                  bool shouldLoad = std::find_if(texturesToLoad.begin(),
                                                 texturesToLoad.end(), 
                                                 [currentTexturePath](const std::pair<std::string, ImageLoadOverride> &pair)
                  {
                    return currentTexturePath.string() == pair.first;
                  }) == texturesToLoad.end();

                  if (!assetCache.has<Image2DAsset>(texName + "_m") && shouldLoad)
                    texturesToLoad.emplace_back(currentTexturePath.string(), ImageLoadOverride::MetalnessRoughness);
                }
              }
            }
          }
        }
        asyncModelQueue.pop();
      }

      for (auto& [texturePath, overload] : texturesToLoad)
        loadImageAsync(texturePath, Texture2DParams(), overload);
    }

    void
    asyncLoadModel(const std::filesystem::path &filepath, const std::string &name,
                   uint entityID, Scene* activeScene)
    {
      // Check if the file is valid or not.
      std::ifstream test(filepath);
      if (!test)
      {
        Logs::log("Error, file " + filepath.string() + " cannot be opened.");
        return;
      }

      auto& assetCache = Application::getInstance()->getAssetCache();
      bool hasAsset = assetCache.has<ModelAsset>(name);

      auto loaderImpl = [](const std::filesystem::path &filepath, const std::string &name,
                           uint entityID, Scene* activeScene, bool hasAsset)
      {
        if (!hasAsset)
        {
          ModelAsset* loadable = new ModelAsset();
          loadable->load(filepath);

          asyncModelQueue.push({ loadable, name, filepath, activeScene, entityID });
        }
        else
          asyncModelQueue.push({ nullptr, name, filepath, activeScene, entityID });
      };

      JobSystem::push(loaderImpl, filepath, name, entityID, activeScene, hasAsset);
    }

    //--------------------------------------------------------------------------
    // Textures.
    //--------------------------------------------------------------------------
    ThreadSafeQueue<ImageData2D> asyncTexQueue;

    void
    bulkGenerateTextures()
    {
      auto& assetCache = Application::getInstance()->getAssetCache();

      while (!asyncTexQueue.empty())
      {
        ImageData2D& image = asyncTexQueue.front();

        Texture2D* outTex = nullptr;
        if (!assetCache.has<Image2DAsset>(image.name))
          outTex = assetCache.emplace<Image2DAsset>(image.filepath, image.name, image.overload)->getTexture();
        else
        {
          stbi_image_free(image.data);
          asyncTexQueue.pop();
          continue;
        }

        auto tempParams = Texture2D::getDefaultColourParams();
        switch (image.n)
        {
          case 1:
          {
            tempParams.format = TextureFormats::Red;
            tempParams.internal = TextureInternalFormats::Red;
            break;
          }
          case 2:
          {
            tempParams.format = TextureFormats::RG;
            tempParams.internal = TextureInternalFormats::RG;
            break;
          }
          case 3:
          {
            tempParams.format = TextureFormats::RGB;
            tempParams.internal = TextureInternalFormats::RGB;
            break;
          }
          case 4:
          {
            tempParams.format = TextureFormats::RGBA;
            tempParams.internal = TextureInternalFormats::RGBA;
            break;
          }
          default: break;
        }
        tempParams.dataType = TextureDataType::Bytes;
        tempParams.minFilter = TextureMinFilterParams::LinearMipMapLinear;

        outTex->setSize(image.width, image.height);
        outTex->setParams(tempParams);
        outTex->loadData(reinterpret_cast<unsigned char*>(image.data));
        outTex->generateMips();

        Logs::log("Loaded texture: " + image.name + " " +
                  "(W: " + std::to_string(image.width) + ", H: " +
                  std::to_string(image.height) + ", N: "
                  + std::to_string(image.n) + ").");

        stbi_image_free(image.data);
        asyncTexQueue.pop();
      }
    }

    void
    loadImageAsync(const std::filesystem::path &filepath, const Texture2DParams &params, 
                   ImageLoadOverride overload)
    {
      // Check if the file is valid or not.
      std::ifstream test(filepath);
      if (!test)
      {
        Logs::log("Error, file " + filepath.string() + " cannot be opened.");
        return;
      }

      if (filepath.extension().string() == ".dds")
      {
        Logs::log("Error loading " + filepath.filename().string() + ": .dds files are not supported.");
        return;
      }

      if (overload == ImageLoadOverride::MetalnessRoughness)
      {
        auto loaderImpl = [](const std::filesystem::path &path, const Texture2DParams &params)
        {
          auto eventDispatcher = EventDispatcher::getInstance();
          eventDispatcher->queueEvent(new GuiEvent(GuiEventType::StartSpinnerEvent, path.string()));
        
          unsigned char* data;
          ImageData2D outImageM;
          ImageData2D outImageR;
          outImageM.params = params;
          outImageR.params = params;
        
          outImageM.name = path.filename().string() + "_m";
          outImageR.name = path.filename().string() + "_r";
        
          outImageM.filepath = path.string();
          outImageR.filepath = path.string();
        
          outImageM.overload = ImageLoadOverride::MetalnessRoughness;
          outImageR.overload = ImageLoadOverride::MetalnessRoughness;

          // Load the image.
          stbi_set_flip_vertically_on_load(true);
          int channels = 0;
          data = stbi_load(outImageM.filepath.c_str(), &outImageM.width, &outImageM.height, &channels, 0);
          outImageR.width = outImageM.width;
          outImageR.height = outImageM.height;
          outImageR.n = 1;
          outImageM.n = 1;
        
          if (!data || channels <= 0)
          {
            stbi_image_free(data);
            return;
          }

          unsigned char* dataM = new unsigned char[outImageM.width * outImageM.height];
          unsigned char* dataR = new unsigned char[outImageM.width * outImageM.height];
          
          // Loop over the pixels and assign the metalness and roughness textures.
          for (std::size_t i = 0; i < outImageR.width * outImageR.height; i++)
          {
            dataM[i] = data[channels * i + 2];
            dataR[i] = data[channels * i + 1];
          }
  
          outImageM.data = reinterpret_cast<void*>(dataM);
          outImageR.data = reinterpret_cast<void*>(dataR);

          asyncTexQueue.push(outImageM);
          asyncTexQueue.push(outImageR);
        
          stbi_image_free(data);
          eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
        };

        JobSystem::push(loaderImpl, filepath, params);
      }
      else
      {
        auto loaderImpl = [](const std::filesystem::path &path, const Texture2DParams &params)
        {
          auto eventDispatcher = EventDispatcher::getInstance();
          eventDispatcher->queueEvent(new GuiEvent(GuiEventType::StartSpinnerEvent, path.string()));
        
          ImageData2D outImage;
          outImage.params = params;
        
          std::string filename = path.filename().string();
        
          outImage.name = filename;
          outImage.filepath = path.string();
          outImage.overload = ImageLoadOverride::None;
        
          // Load the image.
          stbi_set_flip_vertically_on_load(true);
          outImage.data = stbi_load(outImage.filepath.c_str(), &outImage.width, &outImage.height, &outImage.n, 0);
        
          if (!outImage.data)
          {
            stbi_image_free(outImage.data);
            return;
          }
          asyncTexQueue.push(outImage);
        
          eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
        };

        JobSystem::push(loaderImpl, filepath, params);
      }
    }
  }
}
