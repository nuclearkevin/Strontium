#include "Core/Math.h"

namespace Strontium
{
  BoundingBox
  buildBoundingBox(const glm::vec3 &min, const glm::vec3 &max)
  {
    BoundingBox outBox;
    glm::vec3 bb[8] =
    {
      // Min z corners.
      { max.x, max.y, min.z },
      { min.x, max.y, min.z },
      { max.x, min.y, min.z },
      { min.x, min.y, min.z },

      // Max z corners.
      { max.x, max.y, max.z },
      { min.x, max.y, max.z },
      { max.x, min.y, max.z },
      { min.x, min.y, max.z }
    };

    for (unsigned i = 0; i < 8; i++)
      outBox.corners[i] = bb[i];

    outBox.min = min;
    outBox.max = max;

    // Compute the planes which make up the bounding box.
    glm::vec3 edge1, edge2;
    Plane back;
    edge2 = outBox.corners[3] - outBox.corners[2];
    edge1 = outBox.corners[0] - outBox.corners[2];
    back.normal = glm::normalize(glm::cross(edge2, edge1));
    back.d = glm::dot(outBox.corners[0], back.normal);
    back.point = outBox.corners[0];

    Plane front;
    edge2 = outBox.corners[5] - outBox.corners[4];
    edge1 = outBox.corners[6] - outBox.corners[4];
    front.normal = glm::normalize(glm::cross(edge2, edge1));
    front.d = glm::dot(outBox.corners[4], front.normal);
    front.point = outBox.corners[4];

    Plane left;
    edge2 = outBox.corners[5] - outBox.corners[7];
    edge1 = outBox.corners[3] - outBox.corners[7];
    left.normal = glm::normalize(glm::cross(edge2, edge1));
    left.d = glm::dot(outBox.corners[1], left.normal);
    left.point = outBox.corners[1];

    Plane right;
    edge2 = outBox.corners[2] - outBox.corners[6];
    edge1 = outBox.corners[4] - outBox.corners[6];
    right.normal = glm::normalize(glm::cross(edge2, edge1));
    right.d = glm::dot(outBox.corners[0], right.normal);
    right.point = outBox.corners[0];

    Plane bottom;
    edge2 = outBox.corners[7] - outBox.corners[6];
    edge1 = outBox.corners[2] - outBox.corners[6];
    bottom.normal = glm::normalize(glm::cross(edge2, edge1));
    bottom.d = glm::dot(outBox.corners[2], bottom.normal);
    bottom.point = outBox.corners[2];

    Plane top;
    edge2 = outBox.corners[5] - outBox.corners[1];
    edge1 = outBox.corners[0] - outBox.corners[1];
    top.normal = glm::normalize(glm::cross(edge2, edge1));
    top.d = glm::dot(outBox.corners[0], top.normal);
    top.point = outBox.corners[0];

    outBox.sides[0] = back;
    outBox.sides[1] = front;
    outBox.sides[2] = left;
    outBox.sides[3] = right;
    outBox.sides[4] = bottom;
    outBox.sides[5] = top;

    return outBox;
  }

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
    back.point = outFrustum.corners[0];

    Plane front;
    front.normal = glm::normalize(-1.0f * camViewVec);
    front.d = glm::dot(outFrustum.corners[4], front.normal);
    front.point = outFrustum.corners[4];

    Plane left;
    edge2 = outFrustum.corners[5] - outFrustum.corners[7];
    edge1 = outFrustum.corners[3] - outFrustum.corners[7];
    left.normal = glm::normalize(glm::cross(edge2, edge1));
    left.d = glm::dot(outFrustum.corners[1], left.normal);
    left.point = outFrustum.corners[1];

    Plane right;
    edge2 = outFrustum.corners[2] - outFrustum.corners[6];
    edge1 = outFrustum.corners[4] - outFrustum.corners[6];
    right.normal = glm::normalize(glm::cross(edge2, edge1));
    right.d = glm::dot(outFrustum.corners[0], right.normal);
    right.point = outFrustum.corners[0];

    Plane bottom;
    edge2 = outFrustum.corners[7] - outFrustum.corners[6];
    edge1 = outFrustum.corners[2] - outFrustum.corners[6];
    bottom.normal = glm::normalize(glm::cross(edge2, edge1));
    bottom.d = glm::dot(outFrustum.corners[2], bottom.normal);
    bottom.point = outFrustum.corners[2];

    Plane top;
    edge2 = outFrustum.corners[5] - outFrustum.corners[1];
    edge1 = outFrustum.corners[0] - outFrustum.corners[1];
    top.normal = glm::normalize(glm::cross(edge2, edge1));
    top.d = glm::dot(outFrustum.corners[0], top.normal);
    top.point = outFrustum.corners[0];

    outFrustum.sides[0] = back;
    outFrustum.sides[1] = front;
    outFrustum.sides[2] = left;
    outFrustum.sides[3] = right;
    outFrustum.sides[4] = bottom;
    outFrustum.sides[5] = top;

    return outFrustum;
  }

  Frustum
  buildCameraFrustum(const glm::mat4 &viewProj, const glm::vec3 &viewVec)
  {
    glm::mat4 camInvVP = glm::inverse(viewProj);

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
    back.normal = glm::normalize(viewVec);
    back.d = glm::dot(outFrustum.corners[0], back.normal);
    back.point = outFrustum.corners[0];

    Plane front;
    front.normal = glm::normalize(-1.0f * viewVec);
    front.d = glm::dot(outFrustum.corners[4], front.normal);
    front.point = outFrustum.corners[4];

    Plane left;
    edge2 = outFrustum.corners[5] - outFrustum.corners[7];
    edge1 = outFrustum.corners[3] - outFrustum.corners[7];
    left.normal = glm::normalize(glm::cross(edge2, edge1));
    left.d = glm::dot(outFrustum.corners[1], left.normal);
    left.point = outFrustum.corners[1];

    Plane right;
    edge2 = outFrustum.corners[2] - outFrustum.corners[6];
    edge1 = outFrustum.corners[4] - outFrustum.corners[6];
    right.normal = glm::normalize(glm::cross(edge2, edge1));
    right.d = glm::dot(outFrustum.corners[0], right.normal);
    right.point = outFrustum.corners[0];

    Plane bottom;
    edge2 = outFrustum.corners[7] - outFrustum.corners[6];
    edge1 = outFrustum.corners[2] - outFrustum.corners[6];
    bottom.normal = glm::normalize(glm::cross(edge2, edge1));
    bottom.d = glm::dot(outFrustum.corners[2], bottom.normal);
    bottom.point = outFrustum.corners[2];

    Plane top;
    edge2 = outFrustum.corners[5] - outFrustum.corners[1];
    edge1 = outFrustum.corners[0] - outFrustum.corners[1];
    top.normal = glm::normalize(glm::cross(edge2, edge1));
    top.d = glm::dot(outFrustum.corners[0], top.normal);
    top.point = outFrustum.corners[0];

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
  boundingBoxInFrustum(const Frustum &frustum, const glm::vec3 min, const glm::vec3 max)
  {
    auto bb = buildBoundingBox(min, max);

    // First test to see if the corners of the box are in the frustum.
    unsigned int planeTest = 0;
    for (unsigned int i = 0; i < 8; i++)
    {
      for (unsigned int j = 0; j < 6; j++)
      {
        GLfloat distance = signedPlaneDistance(frustum.sides[j], bb.corners[i]);
        if (distance >= 0.0f)
          planeTest++;
      }

      if (planeTest == 6)
        return true;
      planeTest = 0;
    }

    // Next, test to see if the corners of the frustum are in the box.
    planeTest = 0;
    for (unsigned i = 0; i < 8; i++)
    {
      for (unsigned j = 0; j < 6; j++)
      {
        GLfloat distance = signedPlaneDistance(bb.sides[j], frustum.corners[i]);
        if (distance >= 0.0f)
          planeTest++;
      }

      if (planeTest == 6)
        return true;
      planeTest = 0;
    }

    // Reject if both fail. No volume intersections.
    return false;
  }
}
