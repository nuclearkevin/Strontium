#include "Graphics/Meshes.h"

// Project includes.
#include "Core/Logs.h"

#include "Graphics/Renderer.h"

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
    , globalBufferLocation(0u)
  { }

  Mesh::Mesh(const std::string &name, const std::vector<PackedVertex> &vertices,
             const std::vector<uint> &indices, Model* parent)
    : loaded(true)
    , skinned(false)
    , drawable(false)
    , data(vertices)
    , indices(indices)
    , globalBufferLocation(0u)
    , name(name)
    , parent(parent)
    , maxPos(std::numeric_limits<float>::min())
    , minPos(std::numeric_limits<float>::max())
    , localTransform(1.0f)
  { }

  Mesh::~Mesh()
  { }

  bool 
  Mesh::init()
  {
    bool success = false;
    if (this->loaded && !this->drawable)
    {
      Renderer3D::addMeshToCache(*this);
      this->drawable = true;

      success = true;
    }
    
    return success;
  }
}
