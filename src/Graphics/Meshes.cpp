//Header includes.
#include "Graphics/Meshes.h"

// OpenGL includes.
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

namespace SciRenderer
{
  // Constructors
  Mesh::Mesh()
    : loaded(false)
    , modelMatrix(glm::mat4(1.0f))
    , hasUVs(false)
  {
  }

  Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices)
    : loaded(true)
    , data(vertices)
    , indices(indices)
    , modelMatrix(glm::mat4(1.0f))
    , hasUVs(false)
  {
  }

  // Loading function for .obj files.
  void
  Mesh::loadOBJFile(const char* filepath, bool computeTBN)
  {
    tinyobj::ObjReader objReader;
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.mtl_search_path = "";

    if (!objReader.ParseFromFile(filepath, readerConfig))
    {
      if (!objReader.Error().empty())
      {
        std::cerr << "Error loading file (" << filepath << "): "
                  << objReader.Error() << std::endl;
      }
    }

    tinyobj::attrib_t attrib             = objReader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = objReader.GetShapes();
    this->materials                      = objReader.GetMaterials();

    // Get the vertex components.
    for (unsigned i = 0; i < attrib.vertices.size(); i+=3)
    {
      Vertex temp;
      temp.position = glm::vec4(attrib.vertices[i],
                                attrib.vertices[i + 1],
                                attrib.vertices[i + 2],
                                1.0f);
      this->data.push_back(temp);
    }

    // Get the indices.
    unsigned vertexOffset = 0;
    for (unsigned i = 0; i < shapes.size(); i++)
    {
      for (unsigned j = 0; j < shapes[i].mesh.indices.size(); j++)
      {
        this->indices.push_back(shapes[i].mesh.indices[j].vertex_index
                                + vertexOffset);
      }
      vertexOffset += shapes[i].mesh.indices.size();
    }

    // Compute the normal vectors if the obj has none.
    vertexOffset = 0;
    if (attrib.normals.size() == 0)
    {
      std::cout << "Computing vertex normals...";
      this->computeNormals();
      std::cout << " Done!" << std::endl;
    }
    else if (attrib.normals.size() > 0)
    {
      glm::vec3 temp;
      for (unsigned i = 0; i < shapes.size(); i++)
      {
        for (unsigned j = 0; j < shapes[i].mesh.indices.size(); j++)
        {
          temp.x = attrib.normals[3 * (shapes[i].mesh.indices[j].normal_index)];
          temp.y = attrib.normals[3 * (shapes[i].mesh.indices[j].normal_index) + 1];
          temp.z = attrib.normals[3 * (shapes[i].mesh.indices[j].normal_index) + 2];
          this->data[shapes[i].mesh.indices[j].vertex_index].normal = temp;
        }
      }
    }

    // Get the UVs.
    if (attrib.texcoords.size() > 0)
    {
      glm::vec2 temp;
      for (unsigned i = 0; i < shapes.size(); i++)
      {
        for (unsigned j = 0; j < shapes[i].mesh.indices.size(); j++)
        {
          temp.x = attrib.texcoords[2 * (shapes[i].mesh.indices[j].texcoord_index)];
          temp.y = attrib.texcoords[2 * (shapes[i].mesh.indices[j].texcoord_index) + 1];
          this->data[shapes[i].mesh.indices[j].vertex_index].uv = temp;
        }
      }
      this->hasUVs = true;
    }

    // Get the colours for each vertex if they exist.
    if (attrib.colors.size() > 0)
    {
      glm::vec3 temp;
      for (unsigned i = 0; i < shapes.size(); i++)
      {
        for (unsigned j = 0; j < shapes[i].mesh.indices.size(); j++)
        {
          temp.x = attrib.colors[2 * (shapes[i].mesh.indices[j].vertex_index)];
          temp.y = attrib.colors[2 * (shapes[i].mesh.indices[j].vertex_index) + 1];
          temp.z = attrib.colors[2 * (shapes[i].mesh.indices[j].vertex_index) + 2];
          this->data[shapes[i].mesh.indices[j].vertex_index].colour = temp;
        }
      }
    }
    else
    {
      for (unsigned i = 0; i < this->data.size(); i++)
        this->data[i].colour = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    if (computeTBN)
    {
      std::cout << "Computing vertex tangents and bitangents...";
      this->computeTBN();
      std::cout << " Done!" << std::endl;
    }

    this->loaded = true;
  }

  // Generate a vertex array object associated with this mesh.
  void
  Mesh::generateVAO(Shader* program)
  {
    if (!this->isLoaded())
      return;
    this->vArray = new VertexArray(&(this->data[0]),
                                   this->data.size() * sizeof(Vertex),
                                   BufferType::Dynamic);
    this->vArray->addIndexBuffer(&(this->indices[0]), this->indices.size(), BufferType::Dynamic);

  	program->addAtribute("vPosition", VEC4, GL_FALSE, sizeof(Vertex), 0);
  	program->addAtribute("vNormal", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal));
  	program->addAtribute("vColour", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, colour));
    program->addAtribute("vTexCoord", VEC2, GL_FALSE, sizeof(Vertex), offsetof(Vertex, uv));
    program->addAtribute("vTangent", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tangent));
    program->addAtribute("vBitangent", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, bitangent));
  }

  // Delete the vertex array object associated with this mesh.
  void
  Mesh::deleteVAO()
  {
    delete this->vArray;
  }

  // Helper function to bulk compute normals.
  void
  Mesh::computeNormals()
  {
    // Temporary variables to make life easier.
    glm::vec3              a, b, c, edgeOne, edgeTwo, cross;
    std::vector<glm::vec3> vertexNormals;
    std::vector<unsigned>  count;

    for (unsigned i = 0; i < this->data.size(); i++)
    {
      vertexNormals.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f));
      count.push_back(0);
    }

    // Compute the normals of each face.
    for (unsigned i = 0; i < this->indices.size(); i+=3)
    {
      a = this->data[this->indices[i]].position;
      b = this->data[this->indices[i + 1]].position;
      c = this->data[this->indices[i + 2]].position;
      count[this->indices[i]] += 1;
      count[this->indices[i + 1]] += 1;
      count[this->indices[i + 2]] += 1;

      // Compute triangle face normals.
      edgeOne = b - a;
      edgeTwo = c - a;
      cross = glm::normalize(glm::cross(edgeOne, edgeTwo));

      // Add the face normal to the vertex normal array.
      vertexNormals[this->indices[i]]     += cross;
      vertexNormals[this->indices[i + 1]] += cross;
      vertexNormals[this->indices[i + 2]] += cross;
    }

    // Loop over the vertex normal array and normalize the vectors.
    for (unsigned i = 0; i < vertexNormals.size(); i++)
    {
      vertexNormals[i][0] /= count[i];
      vertexNormals[i][1] /= count[i];
      vertexNormals[i][2] /= count[i];
      vertexNormals[i] = glm::normalize(vertexNormals[i]);
      this->data[i].normal = vertexNormals[i];
    }
  }

  void
  Mesh::computeTBN()
  {
    glm::vec3 tangent, bitangent, edgeOne, edgeTwo;
    glm::vec2 duvOne, duvTwo;
    Vertex a, b, c;
    float det;

    for (unsigned i = 0; i < this->indices.size(); i+=3)
    {
      a = this->data[this->indices[i]];
      b = this->data[this->indices[i + 1]];
      c = this->data[this->indices[i + 2]];

      edgeOne = b.position - a.position;
      edgeTwo = c.position - a.position;

      duvOne = b.uv - a.uv;
      duvTwo = c.uv - a.uv;

      det = 1.0f / (duvOne.x * duvTwo.y - duvTwo.x * duvOne.y);

      tangent.x = det * (duvTwo.y * edgeOne.x - duvOne.y * edgeTwo.x);
      tangent.y = det * (duvTwo.y * edgeOne.y - duvOne.y * edgeTwo.y);
      tangent.z = det * (duvTwo.y * edgeOne.z - duvOne.y * edgeTwo.z);

      bitangent.x = det * (-duvTwo.x * edgeOne.x + duvOne.x * edgeTwo.x);
      bitangent.y = det * (-duvTwo.x * edgeOne.y + duvOne.x * edgeTwo.y);
      bitangent.z = det * (-duvTwo.x * edgeOne.z + duvOne.x * edgeTwo.z);

      this->data[this->indices[i]].tangent = tangent;
      this->data[this->indices[i + 1]].tangent= tangent;
      this->data[this->indices[i + 2]].tangent= tangent;

      this->data[this->indices[i]].bitangent += bitangent;
      this->data[this->indices[i + 1]].bitangent += bitangent;
      this->data[this->indices[i + 2]].bitangent += bitangent;
    }
  }

  // Helper function to linearly scale each vertex to the screenspace.
  void
  Mesh::normalizeVertices()
  {
    // Computing scaling factor.
    GLfloat maxX = 0.0f;
    GLfloat maxY = 0.0f;
    GLfloat maxZ = 0.0f;

    GLfloat scale;

    // Loop over the vertices and find the maximum vertex components.
    for (auto &vertex : this->data)
    {
      maxX = std::max(vertex.position.x, maxX);
      maxY = std::max(vertex.position.y, maxY);
      maxZ = std::max(vertex.position.z, maxZ);
    }

    scale = std::max(maxX, maxY);
    scale = std::max(scale, maxZ);

    // Scale each component of the vertices.
    for (auto &vertex : this->data)
    {
      vertex.position[0] = vertex.position[0] * (1 / scale);
      vertex.position[1] = vertex.position[1] * (1 / scale);
      vertex.position[2] = vertex.position[2] * (1 / scale);
    }
  }

  void
  Mesh::setModelMatrix(const glm::mat4& model)
  {
    this->modelMatrix = model;
  }

  // Methods for controlling the model matrix.
  void
  Mesh::moveMesh(const glm::vec3 &direction)
  {
    this->modelMatrix = glm::translate(this->modelMatrix, direction);
  }

  void
  Mesh::rotateMesh(const GLfloat &angle, const glm::vec3 &axis)
  {
    this->modelMatrix = glm::rotate(this->modelMatrix, angle, axis);
  }

  void
  Mesh::rotateMesh(const glm::vec3 &eulerAngles)
  {
    glm::quat rot = glm::quat(eulerAngles);
    this->modelMatrix = glm::toMat4(rot) * this->modelMatrix;
  }

  void
  Mesh::scaleMesh(const GLfloat &scale)
  {
    this->modelMatrix = glm::scale(this->modelMatrix, glm::vec3(scale, scale, scale));
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

  // Getters, yay. . .
  std::vector<GLuint>&
  Mesh::getIndices() {return this->indices;}

  std::vector<Vertex>&
  Mesh::getData() {return this->data;}

  VertexArray*
  Mesh::getVAO() {return this->vArray;}

  glm::mat4
  Mesh::getModelMatrix()
  {return this->modelMatrix;}

  bool
  Mesh::isLoaded() {return this->loaded;}

  bool
  Mesh::hasVAO() {return this->vArray != nullptr;}
}
