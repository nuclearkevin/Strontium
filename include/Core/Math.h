#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Graphics/Camera.h"

namespace Strontium
{
  struct Plane
  {
    GLfloat d;
    glm::vec3 point;
    glm::vec3 normal;
  };

  struct BoundingBox
  {
    glm::vec3 corners[8];
    Plane sides[6];
    glm::vec3 min;
    glm::vec3 max;
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

  BoundingBox buildBoundingBox(const glm::vec3 &min, const glm::vec3 &max);
  Frustum buildCameraFrustum(Shared<Camera> camera);
  Frustum buildCameraFrustum(const glm::mat4 &viewProj, const glm::vec3 &viewVec);

  GLfloat signedPlaneDistance(const Plane &plane, const glm::vec3 &point);

  bool sphereInFrustum(const Frustum &frustum, const glm::vec3 center, GLfloat radius);
  bool boundingBoxInFrustum(const Frustum &frustum, const glm::vec3 min, const glm::vec3 max);
}
