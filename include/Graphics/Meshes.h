// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Shaders.h"

// Tiny object loader!
#include "tinyobjloader/tiny_obj_loader.h"

// Assimp so more than just obj files can be loaded.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace SciRenderer
{
  // Vertex datatypes to store vertex attributes.
  struct Vertex
  {
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec3 colour;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    unsigned  id;
  };

  class Mesh
  {
  public:
    // Constructors. Second one may come in handy for generic geometrical primitives.
    Mesh();
    Mesh(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices);

    ~Mesh();

    // Load data from a file.
    void loadOBJFile(const char* filepath, bool computeTBN = true);
    // Generate/delete the vertex array object.
    void generateVAO();
    void generateVAO(Shared<Shader> program);
    void deleteVAO();
    // Compute vertex and surface normals.
    void computeNormals();
    // Compute the tangents and bitangents.
    void computeTBN();
    // Normalize the vertices to the screenspace (-1 -> 1).
    void normalizeVertices();
    // Debug function to dump to the console.
    void dumpMeshData();
    // Set the model matrix for positioning the model.
    void setModelMatrix(const glm::mat4& model);

    // Various functions to abstract the vector math of moving meshes.
    void moveMesh(const glm::vec3 &direction);
    void rotateMesh(const GLfloat &angle, const glm::vec3 &axis);
    void rotateMesh(const glm::vec3 &eulerAngles);
    void scaleMesh(const GLfloat &scale);
    void setColour(const glm::vec3 &colour);

    // Getters.
    inline std::vector<Vertex>& getData() { return this->data; }
    inline std::vector<GLuint>& getIndices() { return this->indices; }
    inline Shared<VertexArray>  getVAO() { return this->vArray; }
    inline glm::mat4            getModelMatrix() { return this->modelMatrix; }

    // Check for states.
    inline bool hasVAO() { return this->vArray != nullptr; }
    inline bool isLoaded() { return this->loaded; }
  protected:
    // Mesh properties.
    bool loaded;
    std::vector<Vertex>              data;
    std::vector<GLuint>              indices;
    std::vector<tinyobj::material_t> materials;
    glm::mat4                        modelMatrix;
    bool                             hasUVs;

    std::string                      meshName;

    // Vertex array object for the mesh data.
    Shared<VertexArray> vArray;
  };
}
