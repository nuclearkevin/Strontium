#type compute
#version 460 core
/*
 * A compute shader to cull lights. Each local invocation culls a single light.
 */

#define MAX_NUM_LIGHTS 1024
#define GROUP_SIZE 1024

layout(local_size_x = GROUP_SIZE, local_size_y = 1) in;

struct TileData
{
  // World space frustum normals, signed distance is packed in the w component.
  vec4 frustumPlanes[6];
  // World space tile AABB, only xyz are used.
  vec4 aabbCenter;
  vec4 aabbExtents;
};

struct PointLight
{
  vec4 positionRadius; // Position (x, y, z), radius (w).
  vec4 colourIntensity; // Colour (x, y, z) and intensity (w).
};

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y). z and w are unused.
};

layout(std140, binding = 0) readonly buffer AABBandFrustum
{
  TileData frustumsAABBs[];
};

layout(std140, binding = 1) readonly buffer PointLights
{
  PointLight pLights[MAX_NUM_LIGHTS];
};

layout(r16i, binding = 0) restrict writeonly uniform iimage1D indices;

// Viewspace sphere-AABB intersection test.
bool sphereIntersectsAABB(vec3 sPosition, float sRadius, TileData aabb,
                          mat4 viewMatrix)
{
  vec3 sVPosition = (viewMatrix * vec4(sPosition, 1.0)).xyz;
  vec3 delta = max(abs(aabb.aabbCenter.xyz - sVPosition) - aabb.aabbExtents.xyz, 0.0.xxx);
  float deltaSq = dot(delta, delta);

  return deltaSq <= sRadius * sRadius;
}

// Worldspace sphere-frustum intersection test.
bool sphereIntersectsFrustum(vec3 sPosition, float sRadius, TileData frustum)
{
  float dist = 0.0;
  for (uint i = 0; i < 6; i++)
  {
    dist = dot(vec4(sPosition, 1.0), frustum.frustumPlanes[i]) + sRadius;
    if (dist <= 0.0)
      break;
  }

  return dist <= 0.0;
}

bool pointLightInTile(PointLight light, TileData data, mat4 viewMatrix)
{
  bool res1 = sphereIntersectsAABB(light.positionRadius.xyz,
                                   light.positionRadius.w, data,
                                   viewMatrix);
  bool res2 = sphereIntersectsFrustum(light.positionRadius.xyz,
                                      light.positionRadius.w, data);
  return res1 && res2;
}

shared TileData data;

shared uint numBinnedLights;
shared int lightIndicesCache[MAX_NUM_LIGHTS];

void main()
{
  // Can't cull more then the maximum number of lights, so excess threads can
  // safely return early.
  const uint lInvoke = uint(gl_LocalInvocationIndex);
  const int numGroups = int(gl_NumWorkGroups.x); // Same as the total number of tiles.
  const int groupID = int(gl_WorkGroupID.x);

  if (lInvoke == 0)
  {
    data = frustumsAABBs[groupID];
    numBinnedLights = 0;
  }

  lightIndicesCache[lInvoke] = -1;
  barrier();

  // Fetch the light for culling.
  PointLight light = pLights[lInvoke];

  // If the light is inside the tile, add it to the cache.
  if (pointLightInTile(light, data, u_viewMatrix))
  {
    uint offset = atomicAdd(numBinnedLights, 1);
    lightIndicesCache[offset] = int(lInvoke);
  }
  barrier();

  // Done culling, remainings threads which don't have to copy data from the
  // cache to the global light list can safely return.
  if (lInvoke >= numBinnedLights)
    return;

  // Copy the data from the cache to the global light list.
  uint offset = uint(groupID) * MAX_NUM_LIGHTS;
  imageStore(indices, int(offset + lInvoke), ivec4(lightIndicesCache[lInvoke]));
}
