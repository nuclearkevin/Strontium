#include "Core/Math.h"

namespace SciRenderer
{
  Frustum
  buildCameraFrustum(Shared<Camera> camera)
  {
    glm::vec3 camViewVec = camera->getCamFront();
    glm::mat4 camView = camera->getViewMatrix();
    glm::mat4 camProj = camera->getProjMatrix();
    glm::mat4 camInvVP = glm::inverse(camProj * camView);

    Frustum outFrustum;

    glm::vec4 frustumCorners[8] =
    {
      // The near face of the camera frustum in NDC.
      { 1.0f, 1.0f, -1.0f, 1.0f },
      { -1.0f, 1.0f, -1.0f, 1.0f },
      { 1.0f, -1.0f, -1.0f, 1.0f },
      { -1.0f, -1.0f, -1.0f, 1.0f },

      // The far face of the camera frustum in NDC.
      { 1.0f, 1.0f, 1.0f, 1.0f },
      { -1.0f, 1.0f, 1.0f, 1.0f },
      { 1.0f, -1.0f, 1.0f, 1.0f },
      { -1.0f, -1.0f, 1.0f, 1.0f }
    };

    // Compute the worldspace frustum corners, and the center of the frustum.
    outFrustum.min = glm::vec3(std::numeric_limits<float>::max());
    outFrustum.max = glm::vec3(std::numeric_limits<float>::min());
    for (unsigned int i = 0; i < 8; i++)
    {
      glm::vec4 worldDepthless = camInvVP * frustumCorners[i];
      outFrustum.corners[i] = glm::vec3(worldDepthless / worldDepthless.w);
      outFrustum.min = glm::min(outFrustum.corners[i], outFrustum.min);
      outFrustum.max = glm::max(outFrustum.corners[i], outFrustum.max);
    }
    outFrustum.center = (outFrustum.min + outFrustum.max) / 2.0f;

    outFrustum.bSphereRadius = glm::length(outFrustum.min);
    outFrustum.bSphereRadius = glm::max(outFrustum.bSphereRadius, glm::length(outFrustum.max));

    glm::vec3 edge1, edge2;
    Plane back;
    back.normal = glm::normalize(camViewVec);
    back.d = glm::dot(outFrustum.corners[0], back.normal);

    Plane front;
    front.normal = glm::normalize(-1.0f * camViewVec);
    front.d = glm::dot(outFrustum.corners[4], front.normal);

    Plane left;
    edge2 = outFrustum.corners[5] - outFrustum.corners[7];
    edge1 = outFrustum.corners[3] - outFrustum.corners[7];
    left.normal = glm::normalize(glm::cross(edge2, edge1));
    left.d = glm::dot(outFrustum.corners[1], left.normal);

    Plane right;
    edge2 = outFrustum.corners[2] - outFrustum.corners[6];
    edge1 = outFrustum.corners[4] - outFrustum.corners[6];
    right.normal = glm::normalize(glm::cross(edge2, edge1));
    right.d = glm::dot(outFrustum.corners[0], right.normal);

    Plane bottom;
    edge2 = outFrustum.corners[7] - outFrustum.corners[6];
    edge1 = outFrustum.corners[2] - outFrustum.corners[6];
    bottom.normal = glm::normalize(glm::cross(edge2, edge1));
    bottom.d = glm::dot(outFrustum.corners[2], bottom.normal);

    Plane top;
    edge2 = outFrustum.corners[5] - outFrustum.corners[1];
    edge1 = outFrustum.corners[0] - outFrustum.corners[1];
    top.normal = glm::normalize(glm::cross(edge2, edge1));
    top.d = glm::dot(outFrustum.corners[0], top.normal);

    outFrustum.sides[0] = back;
    outFrustum.sides[1] = front;
    outFrustum.sides[2] = left;
    outFrustum.sides[3] = right;
    outFrustum.sides[4] = bottom;
    outFrustum.sides[5] = top;

    return outFrustum;
  }

  GLfloat
  signedPlaneDistance(const Plane &plane, const glm::vec3 &point)
  {
    return plane.normal.x * point.x + plane.normal.y * point.y + plane.normal.z * point.z - plane.d;
  }

  bool
  sphereInFrustum(const Frustum &frustum, const glm::vec3 center, GLfloat radius)
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      GLfloat distance = signedPlaneDistance(frustum.sides[i], center);

      if (distance + radius < 0.0f)
        return false;
    }

    return true;
  }

  bool
  AABBInFrustum(const Frustum &frustum, const glm::vec3 min, const glm::vec3 max)
  {
    glm::vec3 aabb[8] =
    {
      // Min z corners.
      { min.x, min.y, min.z },
      { min.x, max.y, min.z },
      { max.x, min.y, min.z },
      { max.x, max.y, min.z },
      // Max z corners.
      { min.x, min.y, max.z },
      { min.x, max.y, max.z },
      { max.x, min.y, max.z },
      { max.x, max.y, max.z }
    };

    unsigned int planeTest = 0;
    for (unsigned int i = 0; i < 8; i++)
    {
      for (unsigned int j = 0; j < 6; j++)
      {
        GLfloat distance = signedPlaneDistance(frustum.sides[j], aabb[i]);
        if (distance >= 0.0f)
          planeTest++;
      }

      if (planeTest == 6)
        return true;
      planeTest = 0;
    }

    return false;
  }
}
