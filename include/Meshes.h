// Include guard.
#pragma once

// Macro include file.
#include "SciRenderIncludes.h"

// Project includes.
#include "VertexArray.h"
#include "Shaders.h"

// Tiny object loader!
#include "tiny_obj_loader/tiny_obj_loader.h"

namespace SciRenderer
{
  // Vertex datatypes to store vertex attributes.
  struct Vertex
  {
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec3 colour;
    glm::vec2 uv;
    unsigned  id;
  };

  struct Vertex2D
  {
    glm::vec2 position;
    glm::vec2 uv;
  };

  class Mesh
  {
  public:
    // Constructors.
    Mesh();
    Mesh(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices);
    // Destructor.
    ~Mesh() = default;

    // Load data from a file.
    void loadOBJFile(const char* filepath);
    // Generate/delete the vertex array object.
    void generateVAO(Shader* program);
    void deleteVAO();
    // Compute vertex and surface normals.
    void computeNormals();
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
    std::vector<Vertex>&     getData();
    std::vector<GLuint>&     getIndices();
    VertexArray*             getVAO();
    glm::mat4                getModelMatrix();

    // Check for states.
    bool                    hasVAO();
    bool                    isLoaded();
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
    VertexArray* vArray;
  };

  // Forward declaration of the max/min helper functions.
  GLfloat vertexMin(std::vector<Vertex> vector, unsigned start,
                    unsigned end, unsigned axis);
  GLfloat vertexMax(std::vector<Vertex> vector, unsigned start,
                    unsigned end, unsigned axis);
}
