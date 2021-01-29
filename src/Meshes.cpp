//Header includes.
#include "Meshes.h"

// Constructors
Mesh::Mesh()
  : loaded(false)
{
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices)
  : loaded(true)
  , data(vertices)
  , indices(indices)
{
}

// Loading function for .obj files.
void
Mesh::loadOBJFile(const char* filepath)
{
  printf("Loading .obj file.\n");

  // Temporary data structures to store mesh data.
  std::vector<glm::vec4>  vertices;
  std::vector<glm::vec3>  normals;
  std::vector<glm::uvec3> indices;

  // Buffer variables.
  std::string lineBuffer;
  glm::vec4   tempVec4F;
  glm::uvec3  tempVec3U;

  // File handling meshs and varaibles.
  std::ifstream file(filepath);

  // Boolean variables for reading in index values properly.
  bool hasVT = false;
  bool hasVN = false;

  // Couldn't open the file, quit.
  if (!file.is_open())
    return;

  // Parse each line of the obj file.
  while (std::getline(file, lineBuffer))
  {
    // Line contains a vertex, parse accordingly.
    if (lineBuffer.substr(0, 2) == "v ")
    {
      if (2 <= lineBuffer.size())
      {
        // Variables to store the numbers and deal with scientific notation.
        unsigned startIndex = 2;
        float    mult       = 0.0;
        float    exponent   = 0.0;

        // Loop over the line string to find the number.
        for (int j = 0; j < 3; j++)
        {
          for (unsigned i = startIndex; i < lineBuffer.size(); i++)
          {
            // Detect if the value is in scientific notation.
            if (lineBuffer[i] == 'e')
            {
              // Read in the multiplicative part of the scientific notation.
              mult       = std::stof(lineBuffer.substr(startIndex, i - startIndex));
              startIndex = i + 1;
              //printf("buffer: %ld, startIndex: %d ", lineBuffer.size(), startIndex);
              for (unsigned k = startIndex; k <= lineBuffer.size(); k++)
              {
                // Read in the exponential part of the scientific notation.
                if (lineBuffer[k] == ' ')
                {
                  exponent   = std::stof(lineBuffer.substr(startIndex, k - startIndex));
                  startIndex = k + 1;
                  break;
                }
                else
                {
                  // Hella jank, fix when not stupid.
                  exponent = std::stof(lineBuffer.substr(startIndex, lineBuffer.size() - startIndex));
                }
              }
              break;
            }
            // If the number isn't in scientific notation, read it in normally.
            else if (lineBuffer[i] == ' ')
            {
              mult       = std::stof(lineBuffer.substr(startIndex, i - startIndex - 1));
              startIndex = i + 1;
              break;
            }
            else if (i == (lineBuffer.size() - 1))
            {
              mult = std::stof(lineBuffer.substr(startIndex, i - startIndex + 1));
            }
          }
          // Fill the temporary vec3 coordinate.
          tempVec4F[j] = mult * pow(10, exponent);
          exponent = 0.0;
        }
        tempVec4F[3] = 1.0;
        // Push the temporary vec4 to the vertex array.
        data.push_back({ tempVec4F, glm::vec3 {0.0f, 0.0f, 0.0f} });
      }
    }

    // Line contains a face, parse accordingly.
    if (lineBuffer.substr(0, 2) == "f ")
    {
      unsigned startIndex = 2;

      if (3 <= lineBuffer.size())
      {
        // Loop over the line string to find the number.
        for (int j = 0; j < 3; j++)
        {
          for (unsigned i = startIndex; i < lineBuffer.size(); i++)
          {
            // Read in the index.
            if (lineBuffer[i] == ' ')
            {
              tempVec3U[j] = std::stoi(lineBuffer.substr(startIndex, i - startIndex)) - 1;
              startIndex   = i + 1;
              break;
            }
            else if (lineBuffer[i] == '/')
            {
              // Read in the vertex index.
              tempVec3U[j] = std::stoi(lineBuffer.substr(startIndex, i - startIndex)) - 1;
              startIndex   = i + 1;

              // Parse the texture coordinate next (since it shows up after the
              // vertex coordinate). WIP.
              if (hasVT)
              {
                for (unsigned k = 0; k < lineBuffer.size(); i++)
                {
                  if (lineBuffer[k] == '/' || lineBuffer[k] == ' ')
                  {
                    startIndex = i + 1;
                    break;
                  }
                }
              }

              // Parse the vertex normal next (since it shows up after the
              // texture coordinate). TODO.
              if (hasVT)
              {
                for (unsigned k = 0; k < lineBuffer.size(); k++)
                {
                  if (lineBuffer[k] == '/' || lineBuffer[k] == ' ')
                  {
                    startIndex = i + 1;
                    break;
                  }
                }
              }
            }
            else if (i == lineBuffer.size() - 1)
            {
              if (hasVN) {}
              else if (hasVT) {}
              else {tempVec3U[j] = std::stoi(lineBuffer.substr(startIndex, i - startIndex + 1)) - 1;}
            }
          }
        }

        // Push the indices to the index array.
        this->indices.push_back(tempVec3U[0]);
        this->indices.push_back(tempVec3U[1]);
        this->indices.push_back(tempVec3U[2]);
      }
    }

    // Line contains a texture coordinate, parse accordingly.
    if (lineBuffer.substr(0, 2) == "vt")
    {
      hasVT = true;
    }

    // Line contains a normal, parse accordingly.
    if (lineBuffer.substr(0, 2) == "vn")
    {
      hasVN = true;
    }

    // Line contains a material string, parse accordingly.
    if (lineBuffer.substr(0, 6) == "usemtl")
    {

    }
  }

  // Fill the container mesh.
  this->loaded = true;

  // Compute vertex normal vectors.
  printf("Computing vertex normals.\n");
  this->computeNormals();
}

// Helper function to bulk compute normals.
void
Mesh::computeNormals()
{
  // Temporary variables to make life easier.
  glm::vec3              a, b, c, edgeOne, edgeTwo, cross;
  std::vector<glm::vec3> triangleNormals;
  std::vector<glm::vec3> vertexNormals;
  std::vector<unsigned>  count;

  glm::vec3 zero(0.0f, 0.0f, 0.0f);
  for (unsigned i = 0; i < this->data.size(); i++)
  {
    vertexNormals.push_back(zero);
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
    triangleNormals.push_back(cross);

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

  // Push the vectors to the mesh container.
  this->triangleNormals = triangleNormals;
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
Mesh::normalizeVertices(GLfloat scale)
{
  // Scale each component of the vertices.
  for (auto &vertex : this->data)
  {
    vertex.position[0] = vertex.position[0] * (1 / scale);
    vertex.position[1] = vertex.position[1] * (1 / scale);
    vertex.position[2] = vertex.position[2] * (1 / scale);
  }
}

// Function to apply matrix transforms to an mesh. As its on the CPU, its slow.
void
Mesh::applyTransform(glm::mat4 transform)
{
  // This doesn't work?
  for (auto &vertex : this->data)
    vertex.position = transform * vertex.position;
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
  printf("\nDumping face normals (%ld):\n", this->triangleNormals.size());
  for (unsigned i = 0; i < this->triangleNormals.size(); i++)
  {
    printf("FN%d: (%f, %f, %f)\n", i, this->triangleNormals[i][0],
           this->triangleNormals[i][1],
           this->triangleNormals[i][2]);
  }
}

// Getters, yay. . .
std::vector<GLuint>
Mesh::getIndices() {return this->indices;}

std::vector<glm::vec3>
Mesh::getTriNormals() {return this->triangleNormals;}

std::string
Mesh::getMaterial() {return this->material;}

std::vector<Vertex>
Mesh::getData() {return this->data;}

bool
Mesh::isLoaded() {return this->loaded;}

/*------------------------------------------------------------------------------
Mesh loader and utility functions.
------------------------------------------------------------------------------*/

// Mesh loader, TODO.
std::vector<Mesh*>
meshLoader(const char* filepath)
{
  std::vector<Mesh> outMeshs;
}

// Helper function to recursively find the max of a component of a position
// vector.
GLfloat
vertexMax(std::vector<Vertex> vector, unsigned start,
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
    return std::max(vertexMax(vector, start, middle, axis),
                    vertexMax(vector, middle + 1, end, axis));
  else
    return std::max(vertexMax(vector, start, middle - 1, axis),
                    vertexMax(vector, middle, end, axis));
}

// Helper function to recursively find the min of a component of a position
// vector.
GLfloat
vertexMin(std::vector<Vertex> vector, unsigned start,
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
    return std::min(vertexMin(vector, start, middle, axis),
                    vertexMin(vector, middle + 1, end, axis));
  else
    return std::min(vertexMin(vector, start, middle - 1, axis),
                    vertexMin(vector, middle, end, axis));
}
