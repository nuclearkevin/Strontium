#include "Graphics/Meshes.h"

// Project includes.
#include "Core/Logs.h"

namespace Strontium
{
  Mesh::Mesh(const std::string &name, Model* parent)
    : loaded(false)
    , skinned(false)
    , drawable(false)
    , name(name)
    , parent(parent)
    , maxPos(std::numeric_limits<float>::min())
    , minPos(std::numeric_limits<float>::max())
    , localTransform(1.0f)
    , vertexBuffer(nullptr)
    , indexBuffer(nullptr)
  { }

  Mesh::Mesh(const std::string &name, const std::vector<PackedVertex> &vertices,
             const std::vector<uint> &indices, Model* parent)
    : loaded(true)
    , skinned(false)
    , drawable(false)
    , data(vertices)
    , indices(indices)
    , name(name)
    , parent(parent)
    , maxPos(std::numeric_limits<float>::min())
    , minPos(std::numeric_limits<float>::max())
    , localTransform(1.0f)
    , vertexBuffer(nullptr)
    , indexBuffer(nullptr)
  { }

  Mesh::~Mesh()
  { }

  bool 
  Mesh::init()
  {
    bool success = false;
    if (this->isLoaded() && !this->drawable)
    {
      this->vertexBuffer = createUnique<ShaderStorageBuffer>(this->data.data(), this->data.size() * sizeof(PackedVertex), BufferType::Static);
      this->indexBuffer = createUnique<ShaderStorageBuffer>(this->indices.data(), this->indices.size() * sizeof(uint), BufferType::Static);
      this->drawable = true;

      success = true;
    }
    
    return success;
  }
}
