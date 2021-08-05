#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Meshes.h"
#include "Graphics/Textures.h"
#include "Graphics/Material.h"

// Assimp so more than just obj files can be loaded.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// STL includes.
#include <mutex>
#include <tuple>

namespace Strontium
{
  class Scene;

  // Model class
  class Model
  {
  public:
    Model();
    ~Model();

    static std::queue<std::tuple<Model*, Scene*, GLuint>> asyncModelQueue;
    static std::mutex asyncModelMutex;

    // Async load a model (using a separate thread).
    static void bulkGenerateMaterials();
    static void asyncLoadModel(const std::string &filepath, const std::string &name,
                               GLuint entityID, Scene* activeScene);

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
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Mesh> subMeshes;
    bool loaded;

    glm::vec3 minPos;
    glm::vec3 maxPos;

    std::string filepath;
    std::string name;

    friend class Mesh;
  };
}
