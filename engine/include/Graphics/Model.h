#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Assets/Assets.h"
#include "Graphics/Meshes.h"
#include "Graphics/Animations.h"

// Forward declare Assimp garbage.
struct aiScene;
struct aiNode;
struct aiMesh;

namespace Strontium
{
  // Model class
  class Model : public Asset
  {
  public:
    Model();
    Model(const std::string &path);
    ~Model();

    // Load a model.
    void load(const std::string& filepath);
    void unload();

    // Is the model loaded or not.
    bool isLoaded() { return this->loaded; }

    // Get the submeshes for the model.
    glm::vec3& getMinPos() { return this->minPos; }
    glm::vec3& getMaxPos() { return this->maxPos; }
    std::vector<Mesh>& getSubmeshes() { return this->subMeshes; }
    std::vector<Animation>& getAnimations() { return this->storedAnimations; }
    std::unordered_map<std::string, SceneNode>& getSceneNodes() { return this->sceneNodes; }
    std::unordered_map<std::string, uint>& getBoneMap() { return this->boneMap; }
    std::vector<VertexBone>& getBones() { return this->storedBones; }
    glm::mat4& getGlobalInverseTransform() { return this->globalInverseTransform; }
    glm::mat4& getGlobalTransform() { return this->globalTransform; }
    SceneNode& getRootNode() { return this->rootNode; }
    std::string& getFilepath() { return this->filepath; }

    bool hasSkins() { return this->isSkinned; }
  private:
    void processNode(aiNode* node, const aiScene* scene, const std::string &directory, 
                     const glm::mat4 &parentTransform = glm::mat4(1.0f));
    void processMesh(aiMesh* mesh, const aiScene* scene, const std::string &directory, 
                     const glm::mat4& localTransform = glm::mat4(1.0f));

    void addBoneData(unsigned int boneIndex, float boneWeight, Vertex &toMod);

    // Scene information for this model.
    glm::mat4 globalInverseTransform;
    glm::mat4 globalTransform;
    SceneNode rootNode;
    std::unordered_map<std::string, SceneNode> sceneNodes;

    // Submeshes of this model.
    std::vector<Mesh> subMeshes;

    // Animation information for this model.
    std::vector<Animation> storedAnimations;
    std::vector<VertexBone> storedBones;
    std::unordered_map<std::string, uint> boneMap;

    // Is the model loaded or not?
    bool loaded;
    bool isSkinned;

    glm::vec3 minPos;
    glm::vec3 maxPos;

    std::string filepath;
    std::string name;

    friend class Mesh;
  };
}
