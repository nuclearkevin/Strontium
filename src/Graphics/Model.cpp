#include "Graphics/Model.h"

// Project includes.
#include "Core/AssetManager.h"
#include "Core/Logs.h"
#include "Core/Events.h"
#include "Utils/AssimpUtilities.h"

namespace Strontium
{
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

    std::string directory = filepath.substr(0, filepath.find_last_of('/'));
    std::cout << directory << std::endl;

    // Load the model data.
    this->processNode(scene->mRootNode, scene, directory);
    this->loaded = true;

    // Load the model animations.
    if (scene->HasAnimations())
    {
      std::cout << "Animations!" << std::endl;
      this->animations.reserve(scene->mNumAnimations);
      for (unsigned int i = 0; i < scene->mNumAnimations; i++)
      {
        std::cout << scene->mAnimations[i]->mName.C_Str() << std::endl;
        this->animations.emplace_back(scene, i, this);
      }
    }

    eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
    logs->logMessage(LogMessage("Model loaded at path " + filepath));
  }

  // Recursively process all the nodes in the mesh.
  void
  Model::processNode(aiNode* node, const aiScene* scene, const std::string &directory)
  {
    for (GLuint i = 0; i < node->mNumMeshes; i++)
    {
      aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      this->processMesh(mesh, scene, directory);
    }

    for (GLuint i = 0; i < node->mNumChildren; i++)
      this->processNode(node->mChildren[i], scene, directory);
  }

  // Process each individual mesh.
  void
  Model::processMesh(aiMesh* mesh, const aiScene* scene, const std::string &directory)
  {
    std::string meshName = std::string(mesh->mName.C_Str());
    this->subMeshes.emplace_back(meshName, this);
    auto& meshVertices = this->subMeshes.back().getData();
    auto& meshIndicies = this->subMeshes.back().getIndices();

    auto& meshMin = this->subMeshes.back().getMinPos();
    auto& meshMax = this->subMeshes.back().getMaxPos();
    meshMin = glm::vec3(std::numeric_limits<float>::max());
    meshMax = glm::vec3(std::numeric_limits<float>::min());

    auto& materialInfo = this->subMeshes.back().getMaterialInfo();

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

    if (mesh->mMaterialIndex >= 0)
    {
      aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

      aiTextureType type = aiTextureType_DIFFUSE;
      aiString str;
      for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.albedoTexturePath = directory + "/" + str.C_Str();

        for (unsigned int i = 0; i < materialInfo.albedoTexturePath.size(); i++)
          if (materialInfo.albedoTexturePath[i] == '\\')
            materialInfo.albedoTexturePath[i] = '/';
      }

      type = aiTextureType_SPECULAR;
      for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.specularTexturePath = directory + "/" + str.C_Str();

        for (unsigned int i = 0; i < materialInfo.specularTexturePath.size(); i++)
          if (materialInfo.specularTexturePath[i] == '\\')
            materialInfo.specularTexturePath[i] = '/';
      }

      type = aiTextureType_NORMALS;
      for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.normalTexturePath = directory + "/" + str.C_Str();

        for (unsigned int i = 0; i < materialInfo.normalTexturePath.size(); i++)
          if (materialInfo.normalTexturePath[i] == '\\')
            materialInfo.normalTexturePath[i] = '/';
      }

      type = aiTextureType_BASE_COLOR;
      for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.albedoTexturePath = directory + "/" + str.C_Str();

        for (unsigned int i = 0; i < materialInfo.albedoTexturePath.size(); i++)
          if (materialInfo.albedoTexturePath[i] == '\\')
            materialInfo.albedoTexturePath[i] = '/';
      }

      type = aiTextureType_METALNESS;
      for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.metallicTexturePath = directory + "/" + str.C_Str();

        for (unsigned int i = 0; i < materialInfo.metallicTexturePath.size(); i++)
          if (materialInfo.metallicTexturePath[i] == '\\')
            materialInfo.metallicTexturePath[i] = '/';
      }

      type = aiTextureType_DIFFUSE_ROUGHNESS;
      for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.roughnessTexturePath = directory + "/" + str.C_Str();

        for (unsigned int i = 0; i < materialInfo.roughnessTexturePath.size(); i++)
          if (materialInfo.roughnessTexturePath[i] == '\\')
            materialInfo.roughnessTexturePath[i] = '/';
      }

      type = aiTextureType_AMBIENT_OCCLUSION;
      for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.aoTexturePath = directory + "/" + str.C_Str();

        for (unsigned int i = 0; i < materialInfo.aoTexturePath.size(); i++)
          if (materialInfo.aoTexturePath[i] == '\\')
            materialInfo.aoTexturePath[i] = '/';
      }
    }

    auto& meshBones = this->subMeshes.back().getBones();
    // A storage container for the bones actually used by the mesh.
    std::vector<aiBone*> utilizedBones;
    if (mesh->HasBones())
    {
      std::cout << meshName << std::endl;
      std::cout << "Number of bones pre-cull: " << mesh->mNumBones << std::endl;

      // Assign vertex weights and temporary bone IDs.
      for (unsigned int i = 0; i < mesh->mNumBones; i++)
      {
        auto bone = mesh->mBones[i];
        for (unsigned int j = 0; j < bone->mNumWeights; j++)
        {
          auto weight = bone->mWeights[j].mWeight;
          GLuint vertexIndex = bone->mWeights[j].mVertexId;

          if (vertexIndex < meshVertices.size())
          {
            if (meshVertices[vertexIndex].boneIDs.x < 0)
            {
              meshVertices[vertexIndex].boneIDs.x = i;
              meshVertices[vertexIndex].boneWeights.x = weight;
            }
            else if (meshVertices[vertexIndex].boneIDs.y < 0)
            {
              meshVertices[vertexIndex].boneIDs.y = i;
              meshVertices[vertexIndex].boneWeights.y = weight;
            }
            else if (meshVertices[vertexIndex].boneIDs.z < 0)
            {
              meshVertices[vertexIndex].boneIDs.z = i;
              meshVertices[vertexIndex].boneWeights.z = weight;
            }
            else if (meshVertices[vertexIndex].boneIDs.w < 0)
            {
              meshVertices[vertexIndex].boneIDs.w = i;
              meshVertices[vertexIndex].boneWeights.w = weight;
            }
          }
        }
      }

      // Cull the bones to the mesh.
      for (auto& vertex : meshVertices)
      {
        if (vertex.boneIDs.x >= 0)
        {
            if (std::find(utilizedBones.begin(), utilizedBones.end(), mesh->mBones[vertex.boneIDs.x]) == utilizedBones.end())
              utilizedBones.push_back(mesh->mBones[vertex.boneIDs.x]);
        }
        else if (vertex.boneIDs.y >= 0)
        {
          if (std::find(utilizedBones.begin(), utilizedBones.end(), mesh->mBones[vertex.boneIDs.y]) == utilizedBones.end())
            utilizedBones.push_back(mesh->mBones[vertex.boneIDs.y]);
        }
        else if (vertex.boneIDs.z >= 0)
        {
          if (std::find(utilizedBones.begin(), utilizedBones.end(), mesh->mBones[vertex.boneIDs.z]) == utilizedBones.end())
            utilizedBones.push_back(mesh->mBones[vertex.boneIDs.z]);
        }
        else if (vertex.boneIDs.w >= 0)
        {
          if (std::find(utilizedBones.begin(), utilizedBones.end(), mesh->mBones[vertex.boneIDs.w]) == utilizedBones.end())
            utilizedBones.push_back(mesh->mBones[vertex.boneIDs.w]);
        }

        // Reset the bone IDs as they're no longer valid.
        vertex.boneIDs = glm::ivec4(-1);
      }

      // Loop over the bones and reassign bone IDs based off the culled list of bones.
      for (unsigned int i = 0; i < utilizedBones.size(); i++)
      {
        for (unsigned int j = 0; j < utilizedBones[i]->mNumWeights; j++)
        {
          auto weight = utilizedBones[i]->mWeights[j].mWeight;
          GLuint vertexIndex = utilizedBones[i]->mWeights[j].mVertexId;

          if (vertexIndex < meshVertices.size())
          {
            if (meshVertices[vertexIndex].boneIDs.x < 0)
              meshVertices[vertexIndex].boneIDs.x = i;
            else if (meshVertices[vertexIndex].boneIDs.y < 0)
              meshVertices[vertexIndex].boneIDs.y = i;
            else if (meshVertices[vertexIndex].boneIDs.z < 0)
              meshVertices[vertexIndex].boneIDs.z = i;
            else if (meshVertices[vertexIndex].boneIDs.w < 0)
              meshVertices[vertexIndex].boneIDs.w = i;
          }
        }
      }

      // Finally push the bones into their final container.
      meshBones.reserve(utilizedBones.size());
      for (auto& boneData : utilizedBones)
      {
        meshBones.emplace_back();
        meshBones.back().name = boneData->mName.C_Str();
        meshBones.back().boneOffsetMatrix = Utilities::mat4Cast(boneData->mOffsetMatrix);
      }

      std::cout << "Number of bones post-cull: " << meshBones.size() << std::endl;
    }

    this->subMeshes.back().setLoaded(true);
  }
}
