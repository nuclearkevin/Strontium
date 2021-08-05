#include "Graphics/Model.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Core/ThreadPool.h"
#include "Core/Logs.h"
#include "Core/Events.h"
#include "Graphics/Material.h"
#include "Scenes/Entity.h"
#include "Scenes/Components.h"

namespace Strontium
{
  std::queue<std::tuple<Model*, Scene*, GLuint>> Model::asyncModelQueue;
  std::mutex Model::asyncModelMutex;

  void
  Model::bulkGenerateMaterials()
  {
    // This occasionally segfaults... TODO: Figure out a better solution.
    std::lock_guard<std::mutex> modelGuard(asyncModelMutex);

    Logger* logs = Logger::getInstance();

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
              materials.attachMesh(submesh.getName(), MaterialType::PBR);
          }
        }
      }
      asyncModelQueue.pop();
    }
  }

  void
  Model::asyncLoadModel(const std::string &filepath, const std::string &name,
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

      if (!modelAssets->hasAsset(name))
      {
        Model* loadable = new Model();
        loadable->loadModel(filepath);

        modelAssets->attachAsset(name, loadable);

        std::lock_guard<std::mutex> imageGuard(asyncModelMutex);
        asyncModelQueue.push({ loadable, activeScene, entityID });
      }
    };

    workerGroup->push(loaderImpl, filepath, name, entityID, activeScene);
  }

  Model::Model()
    : loaded(false)
    , minPos(std::numeric_limits<float>::max())
    , maxPos(std::numeric_limits<float>::min())
  { }

  Model::~Model()
  { }

  void
  Model::loadModel(const std::string &filepath)
  {
    Logger* logs = Logger::getInstance();
    auto eventDispatcher = EventDispatcher::getInstance();
    eventDispatcher->queueEvent(new GuiEvent(GuiEventType::StartSpinnerEvent, filepath));

    auto flags = aiProcess_CalcTangentSpace | aiProcess_GenNormals
               | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate
               | aiProcess_GenUVCoords | aiProcess_SortByPType;

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filepath, flags);
    if (!scene)
    {
      logs->logMessage(LogMessage("Model failed to load at the path " + filepath +
                                  ", with the error: " + importer.GetErrorString()
                                  + ".", true, true));
      eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
      return;
    }
    else if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      logs->logMessage(LogMessage("Model failed to load at the path " + filepath +
                                  ", with the error: " + importer.GetErrorString()
                                  + ".", true, true));
      eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
      return;
    }

    this->filepath = filepath;

    this->processNode(scene->mRootNode, scene);
    this->loaded = true;
    eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
    logs->logMessage(LogMessage("Model loaded at path " + filepath));
  }

  // Recursively process all the nodes in the mesh.
  void
  Model::processNode(aiNode* node, const aiScene* scene)
  {
    for (GLuint i = 0; i < node->mNumMeshes; i++)
    {
      aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      this->processMesh(mesh, scene);
    }

    for (GLuint i = 0; i < node->mNumChildren; i++)
      this->processNode(node->mChildren[i], scene);
  }

  // Process each individual mesh.
  void
  Model::processMesh(aiMesh* mesh, const aiScene* scene)
  {
    std::string meshName = std::string(mesh->mName.C_Str());
    this->subMeshes.emplace_back(meshName, this);
    auto& meshVertices = this->subMeshes.back().getData();
    auto& meshIndicies = this->subMeshes.back().getIndices();

    auto& meshMin = this->subMeshes.back().getMinPos();
    auto& meshMax = this->subMeshes.back().getMaxPos();
    meshMin = glm::vec3(std::numeric_limits<float>::max());
    meshMax = glm::vec3(std::numeric_limits<float>::min());

    // Get the positions.
    if (mesh->HasPositions())
    {
      meshVertices.reserve(mesh->mNumVertices);

      for (GLuint i = 0; i < mesh->mNumVertices; i++)
        meshVertices.emplace_back();

      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].position.x = mesh->mVertices[i].x;
        meshVertices[i].position.y = mesh->mVertices[i].y;
        meshVertices[i].position.z = mesh->mVertices[i].z;
        meshVertices[i].position.w = 1.0f;

        meshMin = glm::min(meshMin, glm::vec3(meshVertices[i].position));
        meshMax = glm::max(meshMax, glm::vec3(meshVertices[i].position));
      }
    }
    else
    {
      // Nothing that can be done for this mesh as it has no data.
      return;
    }
    this->minPos = glm::min(this->minPos, meshMin);
    this->maxPos = glm::max(this->maxPos, meshMax);

    if (mesh->HasNormals())
    {
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].normal.x = mesh->mNormals[i].x;
        meshVertices[i].normal.y = mesh->mNormals[i].y;
        meshVertices[i].normal.z = mesh->mNormals[i].z;
      }
    }

    // Load the vertex colours in as white. TODO: Load actual vertex colours.
    {
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
        meshVertices[i].colour = glm::vec3(1.0f);
    }

    // Get the UV's, but only supporting a single UV channel for now.
    if (mesh->mTextureCoords[0])
    {
      // Loop over the texture coords and assign them.
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].uv.x = mesh->mTextureCoords[0][i].x;
        meshVertices[i].uv.y = mesh->mTextureCoords[0][i].y;
      }
    }
    else
    {
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].uv = glm::vec2(0.0f);
      }
    }

    // Fetch the tangents and bitangents.
    if (mesh->HasTangentsAndBitangents())
    {
      for (GLuint i = 0; i <mesh->mNumVertices; i++)
      {
        meshVertices[i].tangent.x = mesh->mTangents[i].x;
        meshVertices[i].tangent.y = mesh->mTangents[i].y;
        meshVertices[i].tangent.z = mesh->mTangents[i].z;

        meshVertices[i].bitangent.x = mesh->mBitangents[i].x;
        meshVertices[i].bitangent.y = mesh->mBitangents[i].y;
        meshVertices[i].bitangent.z = mesh->mBitangents[i].z;
      }
    }

    // Fetch the indicies.
    meshIndicies.reserve(mesh->mNumFaces * 3);
    for (GLuint i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];

      for (GLuint j = 0; j < face.mNumIndices; j++)
        meshIndicies.push_back(face.mIndices[j]);
    }

    this->subMeshes.back().setLoaded(true);
  }
}
