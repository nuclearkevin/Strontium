#include "Graphics/Model.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/Events.h"
#include "Utils/AssimpUtilities.h"

// GLM stuff.
#include "glm/gtx/string_cast.hpp"

// Assimp includes.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Strontium
{
  Model::Model()
    : loaded(false)
    , globalInverseTransform(1.0f)
    , globalTransform(1.0f)
    , minPos(std::numeric_limits<float>::max())
    , maxPos(std::numeric_limits<float>::min())
    , isSkinned(false)
  { }

  Model::Model(const std::string &path)
    : loaded(false)
    , globalInverseTransform(1.0f)
    , globalTransform(1.0f)
    , minPos(std::numeric_limits<float>::max())
    , maxPos(std::numeric_limits<float>::min())
    , isSkinned(false)
  {
    this->load(path);
  }

  Model::~Model()
  { }

  void
  Model::load(const std::string &filepath)
  {
    auto eventDispatcher = EventDispatcher::getInstance();
    eventDispatcher->queueEvent(new GuiEvent(GuiEventType::StartSpinnerEvent, filepath));

    auto flags = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_GenUVCoords 
                | aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure 
                | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filepath, flags);
    if (!scene)
    {
      Logs::log("Model failed to load at the path " + filepath +
                ", with the error: " + importer.GetErrorString()
                + ".");
      eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
      return;
    }
    else if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      Logs::log("Model failed to load at the path " + filepath +
                ", with the error: " + importer.GetErrorString()
                + ".");
      eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
      return;
    }

    this->filepath = filepath;

#ifdef WIN32
    std::string directory = filepath.substr(0, filepath.find_last_of('\\'));
#else
    std::string directory = filepath.substr(0, filepath.find_last_of('/'));
#endif
    
    // Load the model data.
    this->globalInverseTransform = glm::inverse(Utilities::mat4ToGLM(scene->mRootNode->mTransformation));
    this->globalTransform = Utilities::mat4ToGLM(scene->mRootNode->mTransformation);
    this->rootNode = SceneNode(scene->mRootNode->mName.C_Str(), Utilities::mat4ToGLM(scene->mRootNode->mTransformation));

    for (uint i = 0; i < scene->mRootNode->mNumChildren; i++)
      this->rootNode.childNames.emplace_back(scene->mRootNode->mChildren[i]->mName.C_Str());
    
    this->processNode(scene->mRootNode, scene, directory);

    // Load in animations.
    if (scene->HasAnimations())
    {
      for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        this->storedAnimations.emplace_back(scene->mAnimations[i], this);
    }

    this->loaded = true;

    eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
    Logs::log("Model loaded at path " + filepath + ".");
  }

  void 
  Model::unload()
  {

  }

  // Recursively process all the nodes in the mesh.
  void
  Model::processNode(aiNode* node, const aiScene* scene, const std::string &directory, 
                     const glm::mat4& parentTransform)
  {
    if (this->sceneNodes.find(node->mName.C_Str()) == this->sceneNodes.end())
    {
      this->sceneNodes.insert(std::make_pair<std::string, SceneNode>(node->mName.C_Str(), SceneNode(node->mName.C_Str(), Utilities::mat4ToGLM(node->mTransformation))));
      for (uint i = 0; i < node->mNumChildren; i++)
        this->sceneNodes[node->mName.C_Str()].childNames.emplace_back(node->mChildren[i]->mName.C_Str());
    }

    auto globalTransform = parentTransform * Utilities::mat4ToGLM(node->mTransformation);

    for (uint i = 0; i < node->mNumMeshes; i++)
    {
      aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      this->processMesh(mesh, scene, directory, globalTransform);
    }

    for (uint i = 0; i < node->mNumChildren; i++)
      this->processNode(node->mChildren[i], scene, directory, globalTransform);
  }

  // Process each individual mesh.
  void
  Model::processMesh(aiMesh* mesh, const aiScene* scene, const std::string &directory, 
                     const glm::mat4& localTransform)
  {
    std::string meshName = std::string(mesh->mName.C_Str());
    this->subMeshes.emplace_back(meshName, this);
    auto& meshVertices = this->subMeshes.back().getData();
    auto& meshIndicies = this->subMeshes.back().getIndices();
    this->subMeshes.back().getTransform() = localTransform;

    auto& meshMin = this->subMeshes.back().getMinPos();
    auto& meshMax = this->subMeshes.back().getMaxPos();
    meshMin = glm::vec3(std::numeric_limits<float>::max());
    meshMax = glm::vec3(std::numeric_limits<float>::min());

    auto& materialInfo = this->subMeshes.back().getMaterialInfo();

    // Get the positions.
    if (mesh->HasPositions())
    {
      meshVertices.reserve(mesh->mNumVertices);

      for (uint i = 0; i < mesh->mNumVertices; i++)
        meshVertices.emplace_back();

      for (uint i = 0; i < mesh->mNumVertices; i++)
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
      for (uint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].normal.x = mesh->mNormals[i].x;
        meshVertices[i].normal.y = mesh->mNormals[i].y;
        meshVertices[i].normal.z = mesh->mNormals[i].z;
      }
    }

    // Get the UV's, but only supporting a single UV channel for now.
    if (mesh->HasTextureCoords(0))
    {
      // Loop over the texture coords and assign them.
      for (uint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].uv.x = mesh->mTextureCoords[0][i].x;
        meshVertices[i].uv.y = mesh->mTextureCoords[0][i].y;
      }
    }
    else
    {
      for (uint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].uv = glm::vec2(0.0f);
      }
    }

    // Fetch the tangents and bitangents.
    if (mesh->HasTangentsAndBitangents())
    {
      for (uint i = 0; i <mesh->mNumVertices; i++)
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
    for (uint i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];

      for (uint j = 0; j < face.mNumIndices; j++)
        meshIndicies.push_back(face.mIndices[j]);
    }

    // Load in the data required to async load material properties.
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

    // Load in vertex bones.
    // Vertex 0 has weights of 0 on specific hardware??
    if (mesh->HasBones())
    {
      this->isSkinned = true;

      for (unsigned int i = 0; i < mesh->mNumBones; i++)
      {
        std::string boneName = mesh->mBones[i]->mName.C_Str();
        glm::mat4 offsetMatrix = Utilities::mat4ToGLM(mesh->mBones[i]->mOffsetMatrix);

        unsigned int boneIndex;
        if (this->boneMap.find(boneName) == this->boneMap.end())
        {
          this->storedBones.emplace_back(boneName, meshName, offsetMatrix);
          this->boneMap[boneName] = this->storedBones.size() - 1;
          boneIndex = this->storedBones.size() - 1;
        }
        else
        {
          boneIndex = this->boneMap.at(boneName);
        }

        for (unsigned int j = 0; j < mesh->mBones[i]->mNumWeights; j++)
        {
          unsigned int vertexIndex = mesh->mBones[i]->mWeights[j].mVertexId;
          float weight = mesh->mBones[i]->mWeights[j].mWeight;
          this->addBoneData(boneIndex, weight, meshVertices[vertexIndex]);
        }
      }
    }

    this->subMeshes.back().setLoaded(true);
  }

  void
  Model::addBoneData(unsigned int boneIndex, float boneWeight, Vertex &toMod)
  {
    for (unsigned int i = 0; i < MAX_BONES_PER_VERTEX; i++)
    {
      if (toMod.boneIDs[i] < 0 || toMod.boneWeights[i] == 0.0f)
      {
        toMod.boneWeights[i] = boneWeight;
        toMod.boneIDs[i] = boneIndex;
        return;
      }
    }
  }
}
