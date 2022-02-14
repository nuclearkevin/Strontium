#include "Utils/AsyncAssetLoading.h"

// Project includes.
#include "Core/Events.h"
#include "Core/ThreadPool.h"
#include "Graphics/Material.h"
#include "Scenes/Entity.h"
#include "Scenes/Components.h"

// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

// STL includes.
#include <mutex>
#include <filesystem>

namespace Strontium
{
  namespace AsyncLoading
  {
    //--------------------------------------------------------------------------
    // Models, materials and meshes.
    //--------------------------------------------------------------------------
    std::queue<std::tuple<Model*, Scene*, uint>> asyncModelQueue;
    std::mutex asyncModelMutex;

    void
    bulkGenerateMaterials()
    {
      // This occasionally segfaults... TODO: Figure out a better solution.
      std::lock_guard<std::mutex> modelGuard(asyncModelMutex);

      Logger* logs = Logger::getInstance();
      auto textureCache = AssetManager<Texture2D>::getManager();

      // Filepaths of the textures that need to be loaded.
      std::vector<std::string> texturesToLoad;
      // Reserve for the worst case scenario.
      texturesToLoad.reserve(asyncModelQueue.size() * 6);

      while (!asyncModelQueue.empty())
      {
        auto [model, activeScene, entityID] = asyncModelQueue.front();
        Entity entity((entt::entity) entityID, activeScene);

        if (entity)
        {
          if (entity.hasComponent<RenderableComponent>())
          {
            auto& rComponent = entity.getComponent<RenderableComponent>();
            auto& materials = rComponent.materials;
            auto& submeshes = model->getSubmeshes();

            if (rComponent.animationHandle != "")
            {
              for (auto& animation : model->getAnimations())
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

                materials.attachMesh(submeshName, MaterialType::PBR);
                auto submeshMaterial = materials.getMaterial(submeshName);

                std::string texName;
                if (submeshTexturePaths.albedoTexturePath != "")
                {
                  submeshMaterial->set(glm::vec3(1.0f), "uAlbedo");
                  texName = submeshTexturePaths.albedoTexturePath.substr(submeshTexturePaths.albedoTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("albedoMap", texName);

                  bool shouldLoad = std::find(texturesToLoad.begin(),
                                              texturesToLoad.end(),
                                              submeshTexturePaths.albedoTexturePath
                                              ) == texturesToLoad.end();

                  if (!textureCache->hasAsset(texName) && shouldLoad)
                    texturesToLoad.emplace_back(submeshTexturePaths.albedoTexturePath);
                }

                if (submeshTexturePaths.roughnessTexturePath != "")
                {
                  submeshMaterial->set(1.0f, "uRoughness");
                  texName = submeshTexturePaths.roughnessTexturePath.substr(submeshTexturePaths.roughnessTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("roughnessMap", texName);

                  bool shouldLoad = std::find(texturesToLoad.begin(),
                                              texturesToLoad.end(),
                                              submeshTexturePaths.roughnessTexturePath
                                              ) == texturesToLoad.end();

                  if (!textureCache->hasAsset(texName) && shouldLoad)
                    texturesToLoad.emplace_back(submeshTexturePaths.roughnessTexturePath);
                }

                if (submeshTexturePaths.metallicTexturePath != "")
                {
                  submeshMaterial->set(1.0f, "uMetallic");
                  texName = submeshTexturePaths.metallicTexturePath.substr(submeshTexturePaths.metallicTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("metallicMap", texName);

                  bool shouldLoad = std::find(texturesToLoad.begin(),
                                              texturesToLoad.end(),
                                              submeshTexturePaths.metallicTexturePath
                                              ) == texturesToLoad.end();

                  if (!textureCache->hasAsset(texName) && shouldLoad)
                    texturesToLoad.emplace_back(submeshTexturePaths.metallicTexturePath);
                }

                if (submeshTexturePaths.aoTexturePath != "")
                {
                  submeshMaterial->set(1.0f, "uAO");
                  texName = submeshTexturePaths.aoTexturePath.substr(submeshTexturePaths.aoTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("aOcclusionMap", texName);

                  bool shouldLoad = std::find(texturesToLoad.begin(),
                                              texturesToLoad.end(),
                                              submeshTexturePaths.aoTexturePath
                                              ) == texturesToLoad.end();

                  if (!textureCache->hasAsset(texName) && shouldLoad)
                    texturesToLoad.emplace_back(submeshTexturePaths.aoTexturePath);
                }

                if (submeshTexturePaths.specularTexturePath != "")
                {
                  submeshMaterial->set(0.04f, "uF0");
                  texName = submeshTexturePaths.specularTexturePath.substr(submeshTexturePaths.specularTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("specF0Map", texName);

                  bool shouldLoad = std::find(texturesToLoad.begin(),
                                              texturesToLoad.end(),
                                              submeshTexturePaths.specularTexturePath
                                              ) == texturesToLoad.end();

                  if (!textureCache->hasAsset(texName) && shouldLoad)
                    texturesToLoad.emplace_back(submeshTexturePaths.specularTexturePath);
                }

                if (submeshTexturePaths.normalTexturePath != "")
                {
                  texName = submeshTexturePaths.normalTexturePath.substr(submeshTexturePaths.normalTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("normalMap", texName);

                  bool shouldLoad = std::find(texturesToLoad.begin(),
                                              texturesToLoad.end(),
                                              submeshTexturePaths.normalTexturePath
                                              ) == texturesToLoad.end();

                  if (!textureCache->hasAsset(texName) && shouldLoad)
                    texturesToLoad.emplace_back(submeshTexturePaths.normalTexturePath);
                }
              }
            }
          }
        }
        asyncModelQueue.pop();
      }

      for (auto& texturePath : texturesToLoad)
        loadImageAsync(texturePath);
    }

    void
    asyncLoadModel(const std::string &filepath, const std::string &name,
                   uint entityID, Scene* activeScene)
    {
      // Fetch the logs.
      Logger* logs = Logger::getInstance();

      // Check if the file is valid or not.
      std::ifstream test(filepath);
      if (!test)
      {
        logs->logMessage(LogMessage("Error, file " + filepath + " cannot be opened.", true, true));
        return;
      }

      // Fetch the thread pool.
      auto workerGroup = ThreadPool::getInstance(2);

      auto loaderImpl = [](const std::string &filepath, const std::string &name,
                           uint entityID, Scene* activeScene)
      {
        auto modelAssets = AssetManager<Model>::getManager();

        Model* loadable;
        if (!modelAssets->hasAsset(name))
        {
          loadable = new Model();
          loadable->load(filepath);

          modelAssets->attachAsset(name, loadable);

          std::lock_guard<std::mutex> imageGuard(asyncModelMutex);
          asyncModelQueue.push({ loadable, activeScene, entityID });
        }
        else
        {
          loadable = modelAssets->getAsset(name);
          std::lock_guard<std::mutex> imageGuard(asyncModelMutex);
          asyncModelQueue.push({ loadable, activeScene, entityID });
        }
      };

      workerGroup->push(loaderImpl, filepath, name, entityID, activeScene);
    }

    //--------------------------------------------------------------------------
    // Textures.
    //--------------------------------------------------------------------------
    std::queue<ImageData2D> asyncTexQueue;
    std::mutex asyncTexMutex;

    void
    bulkGenerateTextures()
    {
      std::lock_guard<std::mutex> imageGuard(asyncTexMutex);

      Logger* logs = Logger::getInstance();
      auto textureCache = AssetManager<Texture2D>::getManager();

      while (!asyncTexQueue.empty())
      {
        ImageData2D& image = asyncTexQueue.front();

        // Generate a 2D texture. Currently supports both bytes and floating point
        // HDR images!
        switch (image.n)
        {
          case 1:
          {
            if (image.isHDR)
            {
              image.params.internal = TextureInternalFormats::R32f;
              image.params.format = TextureFormats::Red;
              image.params.dataType = TextureDataType::Floats;
            }
            else
            {
              image.params.internal = TextureInternalFormats::Red;
              image.params.format = TextureFormats::Red;
              image.params.dataType = TextureDataType::Bytes;
            }
            break;
          }

          case 2:
          {
            if (image.isHDR)
            {
              image.params.internal = TextureInternalFormats::RG32f;
              image.params.format = TextureFormats::RG;
              image.params.dataType = TextureDataType::Floats;
            }
            else
            {
              image.params.internal = TextureInternalFormats::RG;
              image.params.format = TextureFormats::RG;
              image.params.dataType = TextureDataType::Bytes;
            }
            break;
          }

          case 3:
          {
            // If its HDR, needs to be GL_RGBA16F instead of GL_RGB16F. Thanks OpenGL....
            if (image.isHDR)
            {
              float* dataFNew;
              dataFNew = new float[image.width * image.height * 4];
              uint offset = 0;

              for (uint i = 0; i < (image.width * image.height * 4); i+=4)
              {
                // Copy over the data from the image loading.
                dataFNew[i] = ((float*) image.data)[i - offset];
                dataFNew[i + 1] = ((float*) image.data)[i + 1 - offset];
                dataFNew[i + 2] = ((float*) image.data)[i + 2 - offset];
                // Make the 4th component (alpha) equal to 1.0f. Could make this a param :thinking:.
                dataFNew[i + 3] = 1.0f;
                // Increment the offset to we don't segfault. :D
                offset ++;
              }

              image.n = 4;
              image.params.internal = TextureInternalFormats::RGBA32f;
              image.params.format = TextureFormats::RGBA;
              image.params.dataType = TextureDataType::Floats;

              stbi_image_free(image.data);
              image.data = dataFNew;
            }
            else
            {
              image.params.internal = TextureInternalFormats::RGB;
              image.params.format = TextureFormats::RGB;
              image.params.dataType = TextureDataType::Bytes;
            }

            break;
          }

          case 4:
          {
            if (image.isHDR)
            {
              image.params.internal = TextureInternalFormats::RGBA32f;
              image.params.format = TextureFormats::RGBA;
              image.params.dataType = TextureDataType::Floats;
            }
            else
            {
              image.params.internal = TextureInternalFormats::RGBA;
              image.params.format = TextureFormats::RGBA;
              image.params.dataType = TextureDataType::Bytes;
            }

            break;
          }

          default: break;
        }

        image.params.minFilter = TextureMinFilterParams::LinearMipMapLinear;
        Texture2D* outTex = new Texture2D(image.width, image.height, image.params);
        outTex->bind();
        outTex->getFilepath() = image.filepath;

        if (image.isHDR)
          outTex->loadData((float*) image.data);
        else
          outTex->loadData((unsigned char*) image.data);
        outTex->generateMips();

        textureCache->attachAsset(image.name, outTex);

        logs->logMessage(LogMessage("Loaded texture: " + image.name + " " +
                                    "(W: " + std::to_string(image.width) + ", H: " +
                                    std::to_string(image.height) + ", N: "
                                    + std::to_string(image.n) + ").", true, true));

        stbi_image_free(image.data);
        asyncTexQueue.pop();
      }
    }

    void
    loadImageAsync(const std::string &filepath, const Texture2DParams &params)
    {
      // Fetch the logs.
      Logger* logs = Logger::getInstance();

      // Fetch the texture cache.
      auto textureCache = AssetManager<Texture2D>::getManager();

      // Check if the file is valid or not.
      std::ifstream test(filepath);
      if (!test)
      {
        logs->logMessage(LogMessage("Error, file " + filepath + " cannot be opened.", true, true));
        return;
      }

      std::filesystem::path fsPath(filepath);
      
      // Fetch the thread pool and event dispatcher.
      auto workerGroup = ThreadPool::getInstance(2);

      auto loaderImpl = [](const std::filesystem::path& path, const Texture2DParams &params)
      {
        auto eventDispatcher = EventDispatcher::getInstance();
        eventDispatcher->queueEvent(new GuiEvent(GuiEventType::StartSpinnerEvent, path.string()));

        ImageData2D outImage;
        outImage.isHDR = (path.extension().string() == ".hdr");
        outImage.params = params;

        std::string filename = path.filename().string();

        outImage.name = filename;
        outImage.filepath = path.string();

        // Load the image.
        stbi_set_flip_vertically_on_load(true);
        if (outImage.isHDR)
          outImage.data = stbi_loadf(outImage.filepath.c_str(), &outImage.width, &outImage.height, &outImage.n, 0);
        else
          outImage.data = stbi_load(outImage.filepath.c_str(), &outImage.width, &outImage.height, &outImage.n, 0);

        if (!outImage.data)
        {
          stbi_image_free(outImage.data);
          return;
        }

        std::lock_guard<std::mutex> imageGuard(asyncTexMutex);
        asyncTexQueue.push(outImage);

        eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
      };

      workerGroup->push(loaderImpl, fsPath, params);
    }
  }
}
