#include "Graphics/Meshes.h"

// Project includes.
#include "Core/Logs.h"

namespace SciRenderer
{
  Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices,
             const std::vector<Texture2D> &textures, Model* parent)
    : loaded(true)
    , data(vertices)
    , indices(indices)
    , modelMatrix(glm::mat4(1.0f))
    , hasUVs(false)
    , vArray(nullptr)
  { }

  Mesh::~Mesh()
  { }

  void
  Mesh::generateVAO()
  {
    if (!this->isLoaded())
      return;
      
    this->vArray = createUnique<VertexArray>(&(this->data[0]), this->data.size() * sizeof(Vertex), BufferType::Dynamic);
    this->vArray->addIndexBuffer(&(this->indices[0]), this->indices.size(),  BufferType::Dynamic);
  }

  // Generate a vertex array object associated with this mesh.
  void
  Mesh::generateVAO(Shader* program)
  {
    if (!this->isLoaded())
      return;

    this->vArray = createUnique<VertexArray>(&(this->data[0]), this->data.size() * sizeof(Vertex), BufferType::Dynamic);
    this->vArray->addIndexBuffer(&(this->indices[0]), this->indices.size(), BufferType::Dynamic);

  	program->addAtribute("vPosition", AttribType::Vec4, GL_FALSE, sizeof(Vertex), 0);
  	program->addAtribute("vNormal", AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal));
  	program->addAtribute("vColour", AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, colour));
    program->addAtribute("vTexCoord", AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, uv));
    program->addAtribute("vTangent", AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tangent));
    program->addAtribute("vBitangent", AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, bitangent));
  }

  void
  Mesh::setColour(const glm::vec3 &colour)
  {
    for (unsigned i = 0; i < this->data.size(); i++)
      this->data[i].colour = colour;
  }

  // Debugging helper function to dump mesh data to the console.
  void
  Mesh::dumpMeshData()
  {
    printf("Dumping vertex coordinates (%ld):\n", this->data.size());
    for (unsigned i = 0; i < this->data.size(); i++)
    {
      printf("V%d: (%f, %f, %f, %f)\n", i, this->data[i].position[0],
             this->data[i].position[1], this->data[i].position[2],
             this->data[i].position[3]);
    }
    printf("\nDumping vertex normals (%ld):\n", this->data.size());
    for (unsigned i = 0; i < this->data.size(); i++)
    {
      printf("N%d: (%f, %f, %f)\n", i, this->data[i].normal[0], this->data[i].normal[1],
             this->data[i].normal[2]);
    }
    printf("\nDumping indices (%ld):\n", this->indices.size());
    for (unsigned i = 0; i < this->indices.size(); i+=3)
    {
      printf("I%d: (%d, %d, %d)\n", i, this->indices[i], this->indices[i + 1],
             this->indices[i + 2]);
    }
  }
}
