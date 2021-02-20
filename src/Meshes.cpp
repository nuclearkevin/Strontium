//Header includes.
#include "Meshes.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

// OpenGL includes.
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace SciRenderer;

// Constructors
Mesh::Mesh()
  : loaded(false)
  , modelMatrix(glm::mat4(1.0f))
{
}

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices)
  : loaded(true)
  , data(vertices)
  , indices(indices)
  , modelMatrix(glm::mat4(1.0f))
{
}

// Loading function for .obj files.
void
Mesh::loadOBJFile(const char* filepath)
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
  if (attrib.normals.size() == 0)
    this->computeNormals();
  else if (attrib.normals.size() / 3 == this->data.size())
  {
    for (unsigned i = 0; i < this->data.size(); i++)
    {
      this->data[i].normal = glm::vec3(attrib.normals[3 * i],
                                       attrib.normals[3 * i + 1],
                                       attrib.normals[3 * i + 2]);
    }
  }

  // Get the UVs.
  if (attrib.texcoords.size() > 0 && attrib.texcoords.size() / 2 == this->data.size())
  {
    for (unsigned i = 0; i < this->data.size(); i++)
    {
      this->data[i].uv = glm::vec2(attrib.texcoords[2 * i],
                                   attrib.texcoords[2 * i + 1]);
    }
  }

  // Get the colours for each vertex if they exist.
  if (attrib.colors.size() > 0 && attrib.colors.size() / 3 == this->data.size())
  {
    for (unsigned i = 0; i < this->data.size(); i++)
    {
      this->data[i].colour = glm::vec3(attrib.colors[3 * i],
                                       attrib.colors[3 * i + 1],
                                       attrib.colors[3 * i + 2]);
    }
  }
  else
  {
    for (unsigned i = 0; i < this->data.size(); i++)
      this->data[i].colour = glm::vec3(1.0f, 1.0f, 1.0f);
  }

  this->loaded = true;
}

// Generate a vertex array object associated with this mesh.
void
Mesh::generateVAO(Shader* program)
{
  this->vArray = new VertexArray(&(this->data[0]),
                                 this->data.size() * sizeof(Vertex),
                                 DYNAMIC);
  this->vArray->addIndexBuffer(&(this->indices[0]), this->indices.size(), DYNAMIC);

	program->addAtribute("vPosition", VEC4, GL_FALSE, sizeof(Vertex), 0);
	program->addAtribute("vNormal", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal));
	program->addAtribute("vColour", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, colour));
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

// Helper function to linearly scale each vertex to the screenspace.
void
Mesh::normalizeVertices()
{
  // Computing scaling factor.
  GLfloat xScale = std::max(std::abs(vertexMax(this->data, 0,
                                               this->data.size() - 1, 0)),
                            std::abs(vertexMin(this->data, 0,
                                               this->data.size() - 1, 0)));
  GLfloat yScale = std::max(std::abs(vertexMax(this->data, 0,
                                               this->data.size() - 1, 1)),
                            std::abs(vertexMin(this->data, 0,
                                               this->data.size() - 1, 1)));
  GLfloat zScale = std::max(std::abs(vertexMax(this->data, 0,
                                               this->data.size() - 1, 2)),
                            std::abs(vertexMin(this->data, 0,
                                               this->data.size() - 1, 2)));
  GLfloat scale = std::max(xScale, yScale);
  scale         = std::max(scale,  zScale);

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
Mesh::moveMesh(glm::vec3 direction)
{
  this->modelMatrix = glm::translate(this->modelMatrix, direction);
}

void
Mesh::rotateMesh(GLfloat angle, glm::vec3 axis)
{
  this->modelMatrix = glm::rotate(this->modelMatrix, angle, axis);
}

void
Mesh::rotateMesh(glm::vec3 eulerAngles)
{
  glm::quat rot = glm::quat(eulerAngles);
  this->modelMatrix = glm::toMat4(rot) * this->modelMatrix;
}

void
Mesh::scaleMesh(GLfloat scale)
{
  this->modelMatrix = glm::scale(this->modelMatrix, glm::vec3(scale, scale, scale));
}

void
Mesh::setColour(glm::vec3 colour)
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
std::vector<GLuint>
Mesh::getIndices() {return this->indices;}

std::vector<Vertex>
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

/*------------------------------------------------------------------------------
Mesh loader and utility functions.
------------------------------------------------------------------------------*/

// Helper function to recursively find the max of a component of a position
// vector.
GLfloat
SciRenderer::vertexMax(std::vector<Vertex> vector, unsigned start,
                       unsigned end, unsigned axis)
{
  // Base cases.
  if (end == start)
    return vector[end].position[axis];

  if ((end - start) == 1)
    return std::max(vector[end].position[axis], vector[start].position[axis]);

  // Divide.
  unsigned middle = (end + start) / 2;

  // Conquer.
  if ((end + start) % 2 == 0)
    return std::max(SciRenderer::vertexMax(vector, start, middle, axis),
                    SciRenderer::vertexMax(vector, middle + 1, end, axis));
  else
    return std::max(SciRenderer::vertexMax(vector, start, middle - 1, axis),
                    SciRenderer::vertexMax(vector, middle, end, axis));
}

// Helper function to recursively find the min of a component of a position
// vector.
GLfloat
SciRenderer::vertexMin(std::vector<Vertex> vector, unsigned start,
                       unsigned end, unsigned axis)
{
  // Base cases.
  if (end == start)
    return vector[end].position[axis];

  if ((end - start) == 1)
    return std::min(vector[end].position[axis], vector[start].position[axis]);

  // Divide.
  unsigned middle = (end + start) / 2;

  // Conquer.
  if ((end + start) % 2 == 0)
    return std::min(SciRenderer::vertexMin(vector, start, middle, axis),
                    SciRenderer::vertexMin(vector, middle + 1, end, axis));
  else
    return std::min(SciRenderer::vertexMin(vector, start, middle - 1, axis),
                    SciRenderer::vertexMin(vector, middle, end, axis));
}
