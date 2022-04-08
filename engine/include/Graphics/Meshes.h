#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"

namespace Strontium
{
  class Model;

  struct PackedVertex
  {
    glm::vec4 normal; // Uncompressed normal. w is padding.
    glm::vec4 tangent; // Uncompressed tangent. w is padding.
    glm::vec4 position; // Uncompressed position (x, y, z). w is padding.
    glm::vec4 boneWeights; // Uncompressed bone weights.
    glm::ivec4 boneIDs; // Bone IDs.
    glm::vec4 texCoord;

    PackedVertex()
      : normal(0.0f)
      , tangent(0.0f)
      , position(0.0f)
      , boneWeights(0.0f)
      , boneIDs(-1)
      , texCoord(0.0f)
    { }
  };

  // Material info from assimp.
  struct UnloadedMaterialInfo
  {
    std::string albedoTexturePath;
    std::string roughnessTexturePath;
    std::string metallicTexturePath;
    std::string aoTexturePath;
    std::string specularTexturePath;
    std::string normalTexturePath;

    UnloadedMaterialInfo()
      : albedoTexturePath("")
      , roughnessTexturePath("")
      , metallicTexturePath("")
      , aoTexturePath("")
      , specularTexturePath("")
      , normalTexturePath("")
    { }
  };

  class Mesh
  {
  public:
    // Construct an empty mesh.
    Mesh(const std::string &name, Model* parent);
    // Mesh class. Must be loaded in as a part of a parent model.
    Mesh(const std::string &name, const std::vector<PackedVertex> &vertices,
         const std::vector<uint> &indices, Model* parent);
    ~Mesh();
    Mesh(Mesh&&) = default;

    // For rendering.
    bool init();
    ShaderStorageBuffer* getIndexBuffer() { return this->indexBuffer.get(); }
    ShaderStorageBuffer* getVertexBuffer() { return this->vertexBuffer.get(); }
    uint numToRender() const { return this->indices.size(); }

    // Set the loaded state.
    void setLoaded(bool isLoaded) { this->loaded = isLoaded; }

    // Getters.
    std::vector<PackedVertex>& getData() { return this->data; }
    std::vector<uint>& getIndices() { return this->indices; }

    glm::vec3 getMinPos() const { return this->minPos; }
    glm::vec3 getMaxPos() const { return this->maxPos; }
    glm::mat4 getTransform() const { return this->localTransform; }

    std::string getName() const { return this->name; }

    UnloadedMaterialInfo& getMaterialInfo() { return this->materialInfo; }

    // Check for states.
    bool isLoaded() const { return this->loaded; }
    bool isDrawable() const { return this->drawable; }
  protected:
    // Mesh properties.
    bool loaded;
    bool skinned;
    bool drawable;
    std::vector<PackedVertex> data;
    std::vector<uint> indices;

    glm::vec3 minPos;
    glm::vec3 maxPos;
    glm::mat4 localTransform;

    std::string filepath;
    std::string name;

    UnloadedMaterialInfo materialInfo;

    Model* parent;

    Unique<ShaderStorageBuffer> vertexBuffer;
    Unique<ShaderStorageBuffer> indexBuffer;

    friend class Model;
  };
}
