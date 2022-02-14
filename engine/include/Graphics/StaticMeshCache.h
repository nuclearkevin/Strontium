#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Buffers.h"
#include "Graphics/Meshes.h"

namespace Strontium
{
  class StaticMeshCache
  {
  public:
	StaticMeshCache();
	~StaticMeshCache();

	uint registerMesh(const std::vector<Vertex> &verticies, const std::vector<uint> &indices);
	void deregisterMesh(uint meshID);

  private:
	std::stack<uint> availableMeshIDs;
	uint numMeshes;

	VertexBuffer staticVertices;
	IndexBuffer staticIndices;
  };
}