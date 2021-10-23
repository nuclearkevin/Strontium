#include "Core/Math.h"

namespace Strontium
{
  BoundingBox
  buildBoundingBox(const glm::vec3 &min, const glm::vec3 &max)
  {
    BoundingBox outBox;

    outBox.center = (min + max) / 2.0f;
    outBox.extents = glm::vec3(max.x - outBox.center.x, max.y - outBox.center.y,
                               max.z - outBox.center.z);

    return outBox;
  }

  BoundingBox 
  buildBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::mat4& modelMatrix)
  {
    BoundingBox outBox;
    
    auto localCenter = (min + max) / 2.0f;
    auto localExtents = glm::vec3(max.x - localCenter.x, max.y - localCenter.y,
                                  max.z - localCenter.z);

    const glm::vec3 right = modelMatrix[0] * localExtents.x;
    const glm::vec3 up = modelMatrix[1] * localExtents.y;
    const glm::vec3 forward = -modelMatrix[2] * localExtents.z;

    const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
                        std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
                        std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

    const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
                        std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
                        std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

    const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
                        std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
                        std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

    outBox.center = modelMatrix * glm::vec4(localCenter, 1.0f);
    outBox.extents = glm::vec3(newIi, newIj, newIk);

    return outBox;
  }

  Frustum
  buildCameraFrustum(const Camera &camera)
  {
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
      glm::vec4 worldDepthless = camera.invViewProj * frustumCorners[i];
      outFrustum.corners[i] = glm::vec3(worldDepthless / worldDepthless.w);
      outFrustum.min = glm::min(outFrustum.corners[i], outFrustum.min);
      outFrustum.max = glm::max(outFrustum.corners[i], outFrustum.max);
    }
    outFrustum.center = (outFrustum.min + outFrustum.max) / 2.0f;

    outFrustum.bSphereRadius = glm::length(outFrustum.min);
    outFrustum.bSphereRadius = glm::max(outFrustum.bSphereRadius, glm::length(outFrustum.max));

    glm::vec3 edge1, edge2;
    Plane back;
    back.normal = glm::normalize(camera.front);
    back.d = glm::dot(outFrustum.corners[0], back.normal);
    back.point = outFrustum.corners[0];

    Plane front;
    front.normal = glm::normalize(-1.0f * camera.front);
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

  float
  signedPlaneDistance(const Plane &plane, const glm::vec3 &point)
  {
    return plane.normal.x * point.x + plane.normal.y * point.y + plane.normal.z * point.z - plane.d;
  }

  bool 
  boundingBoxOnPlane(const Plane& plane, const BoundingBox& box)
  {
    const float r = box.extents.x * std::abs(plane.normal.x) +
                    box.extents.y * std::abs(plane.normal.y) +
                    box.extents.z * std::abs(plane.normal.z);
    return -r <= signedPlaneDistance(plane, box.center);
  }

  bool
  sphereInFrustum(const Frustum &frustum, const glm::vec3 center, float radius)
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      float distance = signedPlaneDistance(frustum.sides[i], center);

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
    bool inFrustum = true;
    for (uint i = 0; i < 6; i++)
      inFrustum = inFrustum && boundingBoxOnPlane(frustum.sides[i], bb);

    return inFrustum;
  }

  bool boundingBoxInFrustum(const Frustum& frustum, const glm::vec3 min, const glm::vec3 max,
                            const glm::mat4& transform)
  {
    auto bb = buildBoundingBox(min, max, transform);

    // First test to see if the corners of the box are in the frustum.
    unsigned int planeTest = 0;
    bool inFrustum = true;
    for (uint i = 0; i < 6; i++)
        inFrustum = inFrustum && boundingBoxOnPlane(frustum.sides[i], bb);

    return inFrustum;
  }
}
