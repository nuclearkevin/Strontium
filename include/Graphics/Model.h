#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Meshes.h"
#include "Graphics/Animations.h"

// Assimp includes.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// STL includes.
#include <mutex>
#include <tuple>

namespace Strontium
{
  // Model class
  class Model
  {
  public:
    Model();
    ~Model();

    // Load a model.
    void loadModel(const std::string &filepath);

    // Is the model loaded or not.
    bool isLoaded() { return this->loaded; }

    // Get the submeshes for the model.
    glm::vec3& getMinPos() { return this->minPos; }
    glm::vec3& getMaxPos() { return this->maxPos; }
    std::vector<Mesh>& getSubmeshes() { return this->subMeshes; }
    std::string& getFilepath() { return this->filepath; }
  private:
    void processNode(aiNode* node, const aiScene* scene, const std::string &directory);
    void processMesh(aiMesh* mesh, const aiScene* scene, const std::string &directory);

    std::vector<Mesh> subMeshes;
    std::vector<Animation> animations;
    bool loaded;

    glm::vec3 minPos;
    glm::vec3 maxPos;

    std::string filepath;
    std::string name;

    friend class Mesh;
  };
}
