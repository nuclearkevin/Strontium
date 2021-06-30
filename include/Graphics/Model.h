#pragma once

// Macro include file.
#include "SciRenderPCH.h"

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

namespace SciRenderer
{
  // Model class
  class Model
  {
  public:
    Model();
    ~Model();

    static std::queue<std::pair<Model*, ModelMaterial*>> asyncModelQueue;
    static std::mutex asyncModelMutex;

    // Async load a model (using a separate thread).
    static void bulkGenerateMaterials();
    static void asyncLoadModel(const std::string &filepath, const std::string &name, ModelMaterial* materialContainer);

    // Load a model.
    void loadModel(const std::string &filepath);

    // Is the model loaded or not.
    bool isLoaded() { return this->loaded; }

    // Get the submeshes for the model.
    glm::vec3& getMinPos() { return this->minPos; }
    glm::vec3& getMaxPos() { return this->maxPos; }
    std::vector<std::pair<std::string, Shared<Mesh>>>& getSubmeshes() { return this->subMeshes; }
    std::string& getFilepath() { return this->filepath; }
  protected:
    bool loaded;
    std::vector<std::pair<std::string, Shared<Mesh>>> subMeshes;

    glm::vec3 minPos;
    glm::vec3 maxPos;

    std::string filepath;
    std::string name;

  private:
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);

    friend class Mesh;
  };
}
