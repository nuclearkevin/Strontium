#pragma once

// Macro include file.
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
    glm::vec3 colour; // TODO: Consider moving colours out of the vertex struct. Large change required though.
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    Vertex()
      : position(0.0f, 0.0f, 0.0f, 1.0f)
      , normal(0.0f)
      , colour(1.0f)
      , uv(0.0f)
      , tangent(0.0f)
      , bitangent(0.0f)
    { }
  };

  class Mesh
  {
  public:
    // Construct an empty mesh.
    Mesh(const std::string &name, Model* parent);
    // Mesh class. Must be loaded in as a part of a parent model.
    Mesh(const std::string &name, const std::vector<Vertex> &vertices,
         const std::vector<GLuint> &indices, Model* parent);

    Mesh(Mesh&&) = default;

    ~Mesh();

    // Generate/delete the vertex array object.
    void generateVAO();
    void deleteVAO();

    // Debug function to dump to the console.
    void dumpMeshData();

    // Set the loaded state.
    void setLoaded(bool isLoaded) { this->loaded = isLoaded; }

    // Getters.
    std::vector<Vertex>& getData() { return this->data; }
    std::vector<GLuint>& getIndices() { return this->indices; }
    glm::vec3& getMinPos() { return this->minPos; }
    glm::vec3& getMaxPos() { return this->maxPos; }
    VertexArray*  getVAO() { return this->vArray.get(); }
    std::string& getFilepath() { return this->filepath; }
    std::string& getName() { return this->name; }

    // Check for states.
    bool hasVAO() { return this->vArray != nullptr; }
    bool isLoaded() { return this->loaded; }
  protected:
    // Mesh properties.
    bool loaded;
    std::vector<Vertex> data;
    std::vector<GLuint> indices;

    glm::vec3 minPos;
    glm::vec3 maxPos;

    std::string filepath;
    std::string name;

    Model* parent;

    // Vertex array object for the mesh data.
    Unique<VertexArray> vArray;
  };
}
