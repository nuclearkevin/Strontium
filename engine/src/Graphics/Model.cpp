#include "Graphics/Model.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/Events.h"
#include "Core/Math.h"
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
  Model::load(const std::filesystem::path &filepath)
  {
    auto eventDispatcher = EventDispatcher::getInstance();
    eventDispatcher->queueEvent(new GuiEvent(GuiEventType::StartSpinnerEvent, filepath.string()));

    auto flags = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_GenUVCoords 
                | aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure 
                | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filepath.string(), flags);
    if (!scene)
    {
      Logs::log("Model failed to load at the path " + filepath.string() +
                ", with the error: " + importer.GetErrorString()
                + ".");
      eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
      return;
    }
    else if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      Logs::log("Model failed to load at the path " + filepath.string() +
                ", with the error: " + importer.GetErrorString()
                + ".");
      eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
      return;
    }
    
    // Load the model data.
    this->globalInverseTransform = glm::inverse(Utilities::mat4ToGLM(scene->mRootNode->mTransformation));
    this->globalTransform = Utilities::mat4ToGLM(scene->mRootNode->mTransformation);
    this->rootNode = SceneNode(scene->mRootNode->mName.C_Str(), Utilities::mat4ToGLM(scene->mRootNode->mTransformation));

    for (uint i = 0; i < scene->mRootNode->mNumChildren; i++)
      this->rootNode.childNames.emplace_back(scene->mRootNode->mChildren[i]->mName.C_Str());
    
    bool isGLTF = filepath.extension().string() == ".gltf";
    this->processNode(scene->mRootNode, scene, filepath.parent_path().string(), 
                      glm::mat4(1.0f), isGLTF);

    // Load in animations.
    if (scene->HasAnimations())
    {
      for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        this->storedAnimations.emplace_back(scene->mAnimations[i], this);
    }

    this->loaded = true;

    eventDispatcher->queueEvent(new GuiEvent(GuiEventType::EndSpinnerEvent, ""));
    Logs::log("Model loaded at path " + filepath.string() + ".");
  }

  // Recursively process all the nodes in the mesh.
  void
  Model::processNode(aiNode* node, const aiScene* scene, const std::filesystem::path &directory, 
                     const glm::mat4& parentTransform, bool isGLTF)
  {
    if (this->sceneNodes.find(node->mName.C_Str()) == this->sceneNodes.end())
    {
      this->sceneNodes.emplace(std::string(node->mName.C_Str()), SceneNode(node->mName.C_Str(), Utilities::mat4ToGLM(node->mTransformation)));
      for (uint i = 0; i < node->mNumChildren; i++)
        this->sceneNodes[node->mName.C_Str()].childNames.emplace_back(node->mChildren[i]->mName.C_Str());
    }

    auto globalTransform = parentTransform * Utilities::mat4ToGLM(node->mTransformation);

    for (uint i = 0; i < node->mNumMeshes; i++)
    {
      aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      this->processMesh(mesh, scene, directory, globalTransform, isGLTF);
    }

    for (uint i = 0; i < node->mNumChildren; i++)
      this->processNode(node->mChildren[i], scene, directory, globalTransform, isGLTF);
  }

  // Process each individual mesh.
  void
  Model::processMesh(aiMesh* mesh, const aiScene* scene, const std::filesystem::path &directory, 
                     const glm::mat4& localTransform, bool isGLTF)
  {
    std::string meshName = std::string(mesh->mName.C_Str());
    this->subMeshes.emplace_back(meshName, this);
    auto& meshVertices = this->subMeshes.back().data;
    auto& meshIndicies = this->subMeshes.back().indices;
    this->subMeshes.back().localTransform = localTransform;

    auto& meshMin = this->subMeshes.back().minPos;
    auto& meshMax = this->subMeshes.back().maxPos;

    auto& materialInfo = this->subMeshes.back().materialInfo;

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

    if (mesh->HasNormals() && mesh->HasTangentsAndBitangents())
    {
      for (uint i = 0; i < mesh->mNumVertices; i++)
      {
        glm::vec3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        glm::vec3 tangent(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

        meshVertices[i].normal = glm::vec4(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
        meshVertices[i].tangent = glm::vec4(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 0.0f);
      }
    }

    // Get the UV's, but only supporting a single UV channel for now.
    if (mesh->HasTextureCoords(0))
    {
      // Loop over the texture coords and assign them.
      for (uint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].texCoord.x = mesh->mTextureCoords[0][i].x;
        meshVertices[i].texCoord.y = mesh->mTextureCoords[0][i].y;
      }
    }
    else
    {
      for (uint i = 0; i < mesh->mNumVertices; i++)
      {
        meshVertices[i].texCoord = glm::vec4(0.0f);
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
      for (uint i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.albedoTexturePath = (directory / std::filesystem::path(str.C_Str())).string();

        // Grab GLTF specific material parameters.
        aiColor4D colour(1.0f, 1.0f, 1.0f, 1.0f);
        if (isGLTF)
          mat->Get(AI_MATKEY_BASE_COLOR, colour);

        materialInfo.albedoTint = glm::vec4(colour.r, colour.g, colour.b, colour.a);
      }

      type = aiTextureType_SPECULAR;
      for (uint i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.specularTexturePath = (directory / std::filesystem::path(str.C_Str())).string();

        // TODO: F0 tinting.
      }

      type = aiTextureType_NORMALS;
      for (uint i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.normalTexturePath = (directory / std::filesystem::path(str.C_Str())).string();
      }

      type = aiTextureType_BASE_COLOR;
      for (uint i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.albedoTexturePath = (directory / std::filesystem::path(str.C_Str())).string();

        aiColor4D colour(1.0f, 1.0f, 1.0f, 1.0f);
        if (isGLTF)
          mat->Get(AI_MATKEY_BASE_COLOR, colour);

        materialInfo.albedoTint = glm::vec4(colour.r, colour.g, colour.b, colour.a);
      }

      type = aiTextureType_METALNESS;
      for (uint i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.metallicTexturePath = (directory / std::filesystem::path(str.C_Str())).string();

        ai_real metalnessFactor = 1.0f;
        if (isGLTF)
          mat->Get(AI_MATKEY_METALLIC_FACTOR, metalnessFactor);

        materialInfo.metallicScale = metalnessFactor;
      }

      type = aiTextureType_DIFFUSE_ROUGHNESS;
      for (uint i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.roughnessTexturePath = (directory / std::filesystem::path(str.C_Str())).string();

        ai_real roughnessFactor = 1.0f;
        if (isGLTF)
          mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor);

        materialInfo.roughnessScale = roughnessFactor;
      }

      type = aiTextureType_AMBIENT_OCCLUSION;
      for (uint i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.aoTexturePath = (directory / std::filesystem::path(str.C_Str())).string();

        // TODO: AO factor. Assimp doesn't seem to support this.
      }
      
      type = aiTextureType_EMISSIVE;
      for (uint i = 0; i < mat->GetTextureCount(type); i++)
      {
        mat->GetTexture(type, i, &str);
        materialInfo.emissiveTexturePath = (directory / std::filesystem::path(str.C_Str())).string();

        aiColor3D colour(1.0f, 1.0f, 1.0f);
        if (isGLTF)
          mat->Get(AI_MATKEY_COLOR_EMISSIVE, colour);

        materialInfo.emissiveTint = glm::vec3(colour.r, colour.g, colour.b);
      }

      // Need to handle GLTF combiend roughness + metalness textures separately.
      if (isGLTF && materialInfo.roughnessTexturePath == "" && materialInfo.metallicTexturePath == "")
      {
        type = aiTextureType_UNKNOWN;
        for (uint i = 0; i < mat->GetTextureCount(type); i++)
        {
          mat->GetTexture(type, i, &str);
          materialInfo.hasCombinedMR = true;
          materialInfo.metallicTexturePath = (directory / std::filesystem::path(str.C_Str())).string();
          materialInfo.roughnessTexturePath = (directory / std::filesystem::path(str.C_Str())).string();
        }
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
  Model::addBoneData(unsigned int boneIndex, float boneWeight, PackedVertex &toMod)
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
