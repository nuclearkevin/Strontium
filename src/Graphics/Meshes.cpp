#include "Graphics/Meshes.h"

// Project includes.
#include "Core/Logs.h"

namespace SciRenderer
{
  Mesh::Mesh(const std::string &name, Model* parent)
    : loaded(false)
    , name(name)
    , parent(parent)
  { }

  Mesh::Mesh(const std::string &name, const std::vector<Vertex> &vertices,
             const std::vector<GLuint> &indices, Model* parent)
    : loaded(true)
    , data(vertices)
    , indices(indices)
    , vArray(nullptr)
    , name(name)
    , parent(parent)
  { }

  Mesh::~Mesh()
  { }

  void
  Mesh::generateVAO()
  {
    if (!this->isLoaded())
      return;

    this->vArray = createUnique<VertexArray>(this->data.data(), this->data.size() * sizeof(Vertex), BufferType::Dynamic);
    this->vArray->addIndexBuffer(this->indices.data(), this->indices.size(), BufferType::Dynamic);

    this->vArray->addAttribute(0, AttribType::Vec4, GL_FALSE, sizeof(Vertex), 0);
  	this->vArray->addAttribute(1, AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal));
  	this->vArray->addAttribute(2, AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, colour));
    this->vArray->addAttribute(3, AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, uv));
    this->vArray->addAttribute(4, AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tangent));
    this->vArray->addAttribute(5, AttribType::Vec3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, bitangent));
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
