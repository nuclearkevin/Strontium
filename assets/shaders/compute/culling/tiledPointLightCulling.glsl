#type compute
#version 460 core
/*
 * A compute shader to cull lights. Each local invocation culls a single light.
 */

#define MAX_NUM_LIGHTS 1024

#define CULL_WITH_BOTH
//#define CULL_FRUSTUM
//#define CULL_AABB

layout(local_size_x = MAX_NUM_LIGHTS, local_size_y = 1) in;

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
  uint lightingSettings; // Maximum number of point lights.
};

layout(std430, binding = 2) writeonly buffer LightIndices
{
  int indices[];
};

// World-space sphere-AABB intersection test.
bool sphereIntersectsAABB(vec3 sPosition, float sRadius, TileData aabb)
{
  vec3 delta = max(0.0.xxx, abs(aabb.aabbCenter.xyz - sPosition) - aabb.aabbExtents.xyz);
  float deltaSq = dot(delta, delta);

  return deltaSq <= sRadius * sRadius;
}

// World-space sphere-frustum intersection test.
bool sphereIntersectsFrustum(vec3 sPosition, float sRadius, TileData frustum)
{
  float dist = 0.0;
  for (uint i = 0; i < 6; i++)
  {
    dist = dot(vec4(sPosition, 1.0), frustum.frustumPlanes[i]) + sRadius;
    if (dist <= 0.0)
      break;
  }

  return dist > 0.0;
}

bool pointLightInTile(PointLight light, TileData data)
{
  #ifdef CULL_WITH_BOTH
  bool res1 = sphereIntersectsAABB(light.positionRadius.xyz,
                                   light.positionRadius.w, data);
  bool res2 = sphereIntersectsFrustum(light.positionRadius.xyz,
                                      light.positionRadius.w, data);
  return res1 && res2;
  #endif

  #ifdef CULL_FRUSTUM
  return sphereIntersectsFrustum(light.positionRadius.xyz,
                                 light.positionRadius.w, data);
  #endif

  #ifdef CULL_AABB
  return sphereIntersectsAABB(light.positionRadius.xyz,
                              light.positionRadius.w, data);
  #endif
}

shared TileData data;

shared uint numBinnedLights;
shared int lightIndicesCache[MAX_NUM_LIGHTS];

void main()
{
  const uint lInvoke = gl_LocalInvocationID.x;
  const uvec2 totalTiles = gl_NumWorkGroups.xy; // Same as the total number of tiles.
  const uvec2 currentTile = gl_WorkGroupID.xy; // The current tile.
  const uint tileOffset = totalTiles.x * currentTile.y + currentTile.x; // Offset into the tile buffer.

  if (lInvoke == 0)
  {
    data = frustumsAABBs[tileOffset];
    numBinnedLights = 0;
  }

  lightIndicesCache[lInvoke] = -1;
  barrier();

  // Quit early to prevent filling the buffer with garbage lights.
  if (lInvoke >= lightingSettings)
    return;

  // Fetch the light for culling.
  PointLight light = pLights[lInvoke];

  // If the light is inside the tile, add it to the cache.
  if (pointLightInTile(light, data))
  {
    uint offset = atomicAdd(numBinnedLights, 1);
    lightIndicesCache[offset] = int(lInvoke);
  }
  barrier();

  // Have each thread write a light to the global buffer.
  if (lInvoke > numBinnedLights)
    return;

  uint offset = tileOffset * MAX_NUM_LIGHTS;
  if (numBinnedLights != MAX_NUM_LIGHTS && lInvoke == numBinnedLights)
  {
    indices[offset + numBinnedLights] = -1;
  }
  else
  {
    indices[offset + lInvoke] = lightIndicesCache[lInvoke];
  }
}
