#include "Utils/AsyncAssetLoading.h"

// Project includes.
#include "Core/ThreadPool.h"
#include "Graphics/Material.h"
#include "Scenes/Entity.h"
#include "Scenes/Components.h"

// Image loading include.
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

// STL includes.
#include <mutex>

namespace Strontium
{
  namespace AsyncLoading
  {
    //--------------------------------------------------------------------------
    // Models, materials and meshes.
    //--------------------------------------------------------------------------
    std::queue<std::tuple<Model*, Scene*, GLuint>> asyncModelQueue;
    std::mutex asyncModelMutex;

    void
    bulkGenerateMaterials()
    {
      // This occasionally segfaults... TODO: Figure out a better solution.
      std::lock_guard<std::mutex> modelGuard(asyncModelMutex);

      Logger* logs = Logger::getInstance();
      auto textureCache = AssetManager<Texture2D>::getManager();

      while (!asyncModelQueue.empty())
      {
        auto [model, activeScene, entityID] = asyncModelQueue.front();
        Entity entity((entt::entity) entityID, activeScene);

        if (entity)
        {
          if (entity.hasComponent<RenderableComponent>())
          {
            auto& materials = entity.getComponent<RenderableComponent>().materials;
            auto& submeshes = model->getSubmeshes();

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
                  submeshMaterial->getVec3("uAlbedo") = glm::vec3(1.0f);
                  texName = submeshTexturePaths.albedoTexturePath.substr(submeshTexturePaths.albedoTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("albedoMap", texName);
                  if (!textureCache->hasAsset(texName))
                    loadImageAsync(submeshTexturePaths.albedoTexturePath);
                }

                if (submeshTexturePaths.roughnessTexturePath != "")
                {
                  submeshMaterial->getFloat("uRoughness") = 1.0f;
                  texName = submeshTexturePaths.roughnessTexturePath.substr(submeshTexturePaths.roughnessTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("roughnessMap", texName);
                  if (!textureCache->hasAsset(texName))
                    loadImageAsync(submeshTexturePaths.roughnessTexturePath);
                }

                if (submeshTexturePaths.metallicTexturePath != "")
                {
                  submeshMaterial->getFloat("uMetallic") = 1.0f;
                  texName = submeshTexturePaths.metallicTexturePath.substr(submeshTexturePaths.metallicTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("metallicMap", texName);
                  if (!textureCache->hasAsset(texName))
                    loadImageAsync(submeshTexturePaths.metallicTexturePath);
                }

                if (submeshTexturePaths.aoTexturePath != "")
                {
                  submeshMaterial->getFloat("uAO") = 1.0f;
                  texName = submeshTexturePaths.aoTexturePath.substr(submeshTexturePaths.aoTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("aOcclusionMap", texName);
                  if (!textureCache->hasAsset(texName))
                    loadImageAsync(submeshTexturePaths.aoTexturePath);
                }

                if (submeshTexturePaths.specularTexturePath != "")
                {
                  submeshMaterial->getFloat("uF0") = 1.0f;
                  texName = submeshTexturePaths.specularTexturePath.substr(submeshTexturePaths.specularTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("specF0Map", texName);
                  if (!textureCache->hasAsset(texName))
                    loadImageAsync(submeshTexturePaths.specularTexturePath);
                }

                if (submeshTexturePaths.normalTexturePath != "")
                {
                  texName = submeshTexturePaths.normalTexturePath.substr(submeshTexturePaths.normalTexturePath.find_last_of('/') + 1);
                  submeshMaterial->attachSampler2D("normalMap", texName);
                  if (!textureCache->hasAsset(texName))
                    loadImageAsync(submeshTexturePaths.normalTexturePath);
                }
              }
            }
          }
        }
        asyncModelQueue.pop();
      }
    }

    void
    asyncLoadModel(const std::string &filepath, const std::string &name,
                   GLuint entityID, Scene* activeScene)
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
                           GLuint entityID, Scene* activeScene)
      {
        auto modelAssets = AssetManager<Model>::getManager();

        Model* loadable;
        if (!modelAssets->hasAsset(name))
        {
          loadable = new Model();
          loadable->loadModel(filepath);

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

        Texture2D* outTex = new Texture2D(image.width, image.height, image.n, image.params);
        outTex->bind();
        outTex->getFilepath() = image.filepath;

        // Generate a 2D texture. Currently supports both bytes and floating point
        // HDR images!
        if (outTex->n == 1)
        {
          if (image.isHDR)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, image.width, image.height, 0,
                         GL_RED, GL_FLOAT, image.data);
          else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, image.width, image.height, 0,
                         GL_RED, GL_UNSIGNED_BYTE, image.data);
          glGenerateMipmap(GL_TEXTURE_2D);
        }
        else if (outTex->n == 2)
        {
          if (image.isHDR)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, image.width, image.height, 0,
                         GL_RG, GL_FLOAT, image.data);
          else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, image.width, image.height, 0,
                         GL_RG, GL_UNSIGNED_BYTE, image.data);
          glGenerateMipmap(GL_TEXTURE_2D);
        }
        else if (outTex->n == 3)
        {
          // If its HDR, needs to be GL_RGBA16F instead of GL_RGB16F. Thanks OpenGL....
          if (image.isHDR)
          {
            float* dataFNew;
            dataFNew = new float[image.width * image.height * 4];
            GLuint offset = 0;

            for (GLuint i = 0; i < (image.width * image.height * 4); i+=4)
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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, image.width, image.height, 0,
                         GL_RGBA, GL_FLOAT, dataFNew);
            stbi_image_free(dataFNew);
          }
          else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0,
                         GL_RGB, GL_UNSIGNED_BYTE, image.data);
          glGenerateMipmap(GL_TEXTURE_2D);
        }
        else if (outTex->n == 4)
        {
          if (image.isHDR)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, image.width, image.height, 0,
                         GL_RGBA, GL_FLOAT, image.data);
          else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, image.data);
          glGenerateMipmap(GL_TEXTURE_2D);
        }

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

      std::string name = filepath.substr(filepath.find_last_of('/') + 1);
      if (textureCache->hasAsset(name))
        return;

      // Fetch the thread pool and event dispatcher.
      auto workerGroup = ThreadPool::getInstance(2);

      auto loaderImpl = [](const std::string &filepath, const std::string &name, const Texture2DParams &params)
      {
        auto eventDispatcher = EventDispatcher::getInstance();
        eventDispatcher->queueEvent(new GuiEvent(GuiEventType::StartSpinnerEvent, filepath));

        ImageData2D outImage;
        outImage.isHDR = (filepath.substr(filepath.find_last_of("."), 4) == ".hdr");
        outImage.params = params;
        outImage.name = name;
        outImage.filepath = filepath;

        // Load the image.
        stbi_set_flip_vertically_on_load(true);
        if (outImage.isHDR)
          outImage.data = stbi_loadf(filepath.c_str(), &outImage.width, &outImage.height, &outImage.n, 0);
        else
          outImage.data = stbi_load(filepath.c_str(), &outImage.width, &outImage.height, &outImage.n, 0);

        if (!outImage.data)
        {
          stbi_image_free(outImage.data);
          return;
        }

        std::lock_guard<std::mutex> imageGuard(asyncTexMutex);
        asyncTexQueue.push(outImage);

        eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
      };

      workerGroup->push(loaderImpl, filepath, name, params);
    }
  }
}
