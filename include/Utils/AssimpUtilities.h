#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Assimp.
#include <assimp/scene.h>

namespace Strontium
{
  namespace Utilities
  {
    // Convert ASSIMP data types to GLM data types.
    static glm::vec3 vec3Cast(const aiVector3D &v) { return glm::vec3(v.x, v.y, v.z); }
    static glm::vec2 vec2Cast(const aiVector3D &v) { return glm::vec2(v.x, v.y); }
    static glm::quat quatCast(const aiQuaternion &q) { return glm::quat(q.w, q.x, q.y, q.z); }
    static glm::mat4 mat4Cast(const aiMatrix4x4 &m) { return glm::transpose(glm::make_mat4(&m.a1)); }
    static glm::mat3 mat3Cast(const aiMatrix3x3 &m) { return glm::transpose(glm::make_mat3(&m.a1)); }
  }
}
