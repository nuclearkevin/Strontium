#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"

namespace Strontium
{
  class Model;

  // Vertex datatypes to store vertex attributes.
  struct Vertex
  {
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::ivec4 boneIDs;
    glm::vec4 boneWeights;

    Vertex()
      : position(0.0f, 0.0f, 0.0f, 1.0f)
      , normal(0.0f)
      , uv(0.0f)
      , tangent(0.0f)
      , bitangent(0.0f)
      , boneIDs(-1)
      , boneWeights(0.0f)
    { }
  };

  struct PackedVertex
  {
    glm::vec4 position;
    glm::quat tangentFrame; 
    glm::vec4 boneWeights;
    glm::ivec4 boneIndices;
    glm::vec2 texCoords;

    PackedVertex()
      : position(0.0f)
      , tangentFrame()
      , boneWeights(0.0f)
      , boneIndices(-1)
      , texCoords(0.0f)
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
    Mesh(const std::string &name, const std::vector<Vertex> &vertices,
         const std::vector<uint> &indices, Model* parent);
    ~Mesh();
    Mesh(Mesh&&) = default;

    // Generate/delete the vertex array object.
    VertexArray* generateVAO();

    // Set the loaded state.
    void setLoaded(bool isLoaded) { this->loaded = isLoaded; }

    // Getters.
    std::vector<Vertex>& getData() { return this->data; }
    std::vector<uint>& getIndices() { return this->indices; }
    glm::vec3& getMinPos() { return this->minPos; }
    glm::vec3& getMaxPos() { return this->maxPos; }
    glm::mat4& getTransform() { return this->localTransform; }
    VertexArray* getVAO() { return this->vArray.get(); }
    std::string& getFilepath() { return this->filepath; }
    std::string& getName() { return this->name; }
    UnloadedMaterialInfo& getMaterialInfo() { return this->materialInfo; }

    // Check for states.
    bool hasVAO() { return this->vArray != nullptr; }
    bool isLoaded() { return this->loaded; }
  protected:
    // Mesh properties.
    bool loaded;
    bool skinned;
    std::vector<Vertex> data;
    std::vector<uint> indices;

    glm::vec3 minPos;
    glm::vec3 maxPos;
    glm::mat4 localTransform;

    std::string filepath;
    std::string name;

    UnloadedMaterialInfo materialInfo;

    Model* parent;

    // Vertex array object for the mesh data.
    Unique<VertexArray> vArray;
  };
}
