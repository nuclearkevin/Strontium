#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"
#include "Graphics/Material.h"

namespace SciRenderer
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
    unsigned  id;
  };

  class Mesh
  {
  public:
    // Mesh class. Must be loaded in as a part of a parent model.
    Mesh(const std::string &name, const std::vector<Vertex> &vertices,
         const std::vector<GLuint> &indices, Model* parent);

    ~Mesh();

    // Generate/delete the vertex array object.
    void generateVAO();
    void generateVAO(Shader* program);

    void deleteVAO();
    // Debug function to dump to the console.
    void dumpMeshData();

    // Set the mesh colour. TODO: Move to a material class.
    void setColour(const glm::vec3 &colour);

    // Getters.
    std::vector<Vertex>& getData() { return this->data; }
    std::vector<GLuint>& getIndices() { return this->indices; }
    VertexArray*  getVAO() { return this->vArray.get(); }
    Material* getMat() { return this->meshMat.get(); }
    std::string getFilepath() { return this->filepath; }
    std::string getName() { return this->name; }

    // Check for states.
    bool hasVAO() { return this->vArray != nullptr; }
    bool isLoaded() { return this->loaded; }
  protected:
    // Mesh properties.
    bool loaded;
    std::vector<Vertex>              data;
    std::vector<GLuint>              indices;
    bool                             hasUVs;

    std::string                      filepath;
    std::string                      name;

    Model* parent;

    // Vertex array object for the mesh data.
    Unique<VertexArray> vArray;
    // The mesh material.
    Unique<Material> meshMat;
  };
}
