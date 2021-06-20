#include "Graphics/Model.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Core/ThreadPool.h"
#include "Core/Logs.h"
#include "Core/Events.h"
#include "Graphics/Material.h"

namespace SciRenderer
{
  Model::Model()
    : loaded(false)
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

  void
  Model::asyncLoadModel(const std::string &filepath, const std::string &name)
  {
    // Fetch the thread pool.
    auto workerGroup = ThreadPool::getInstance(2);

    auto loaderImpl = [](const std::string &filepath, const std::string &name)
    {
      auto modelAssets = AssetManager<Model>::getManager();

      if (!modelAssets->hasAsset(name))
      {
        Model* loadable = new Model();
        loadable->loadModel(filepath);

        modelAssets->attachAsset(name, loadable);
      }
    };

    workerGroup->push(loaderImpl, filepath, name);
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
    std::vector<Vertex> meshVertices;
    std::vector<GLuint> meshIndicies;

    // Get the positions.
    if (mesh->HasPositions())
    {
      meshVertices.reserve(mesh->mNumVertices);

      for (GLuint i = 0; i < mesh->mNumVertices; i++)
        meshVertices.push_back(Vertex());

      glm::vec4 temp;
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        temp.x = mesh->mVertices[i].x;
        temp.y = mesh->mVertices[i].y;
        temp.z = mesh->mVertices[i].z;
        temp.w = 1.0f;

        meshVertices[i].position = temp;
      }
    }
    else
    {
      // Nothing that can be done for this mesh as it has no data.
      return;
    }

    if (mesh->HasNormals())
    {
      glm::vec3 temp;
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        temp.x = mesh->mNormals[i].x;
        temp.y = mesh->mNormals[i].y;
        temp.z = mesh->mNormals[i].z;

        meshVertices[i].normal = temp;
      }
    }

    // Load the vertex colours in as white. TODO: Load actual vertex colours.
    {
      glm::vec3 temp = glm::vec3(1.0f);
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
        meshVertices[i].colour = temp;
    }


    // Get the UV's, but only supporting a single UV channel for now.
    if (mesh->mTextureCoords[0])
    {
      // Loop over the texture coords and assign them.
      glm::vec2 temp;
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        temp.x = mesh->mTextureCoords[0][i].x;
        temp.y = mesh->mTextureCoords[0][i].y;

        meshVertices[i].uv = temp;
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
      glm::vec3 temp1, temp2;
      for (GLuint i = 0; i <mesh->mNumVertices; i++)
      {
        temp1.x = mesh->mTangents[i].x;
        temp1.y = mesh->mTangents[i].y;
        temp1.z = mesh->mTangents[i].z;

        temp2.x = mesh->mBitangents[i].x;
        temp2.y = mesh->mBitangents[i].y;
        temp2.z = mesh->mBitangents[i].z;

        meshVertices[i].tangent = temp1;
        meshVertices[i].bitangent = temp2;
      }
    }

    // Fetch the indicies.
    for (GLuint i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];

      for (GLuint j = 0; j < face.mNumIndices; j++)
        meshIndicies.push_back(face.mIndices[j]);
    }

    std::string meshName = std::string(mesh->mName.C_Str());
    this->subMeshes.push_back(std::pair
      (meshName, createShared<Mesh>(meshName, meshVertices, meshIndicies, this)));
  }
}
