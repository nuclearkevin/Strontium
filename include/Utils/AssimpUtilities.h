#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Assimp.
#include <assimp/scene.h>

namespace Strontium
{
  namespace Utilities
  {
    // Helper functions to convert between ASSIMP and GLM
    static inline glm::vec3 vec3ToGLM(const aiVector3D &v) { return glm::vec3(v.x, v.y, v.z); }
    static inline glm::vec2 vec2ToGLM(const aiVector3D &v) { return glm::vec2(v.x, v.y); }
    static inline glm::quat quatToGLM(const aiQuaternion &q) { return glm::quat(q.w, q.x, q.y, q.z); }
    static inline glm::mat4 mat4ToGLM(const aiMatrix4x4 &m) { return glm::transpose(glm::make_mat4(&m.a1)); }
    static inline glm::mat3 mat3ToGLM(const aiMatrix3x3 &m) { return glm::transpose(glm::make_mat3(&m.a1)); }
  }
}
