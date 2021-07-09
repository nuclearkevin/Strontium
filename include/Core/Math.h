#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Camera.h"

namespace SciRenderer
{
  struct Plane
  {
    GLfloat d;
    glm::vec3 normal;
  };

  struct Frustum
  {
    glm::vec3 corners[8];
    Plane sides[6];
    glm::vec3 center;
    glm::vec3 min;
    glm::vec3 max;
    GLfloat bSphereRadius;
  };

  Frustum buildCameraFrustum(Shared<Camera> camera);

  GLfloat signedPlaneDistance(const Plane &plane, const glm::vec3 &point);

  bool sphereInFrustum(const Frustum &frustum, const glm::vec3 center, GLfloat radius);
  bool AABBInFrustum(const Frustum &frustum, const glm::vec3 min, const glm::vec3 max);
}
