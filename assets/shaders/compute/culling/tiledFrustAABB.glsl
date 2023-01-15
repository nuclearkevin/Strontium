#type compute
#version 460 core
/*
 * A compute shader to precompute the frustums and AABBs used in tiled
 * deferred rendering.
 */

#define TILE_SIZE 16

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE) in;

layout(binding = 0) uniform sampler2D gDepth;

struct TileData
{
  // World space frustum normals, signed distance is packed in the w component.
  vec4 frustumPlanes[6];
  // View space tile AABB, only xyz are used.
  vec4 aabbCenter;
  vec4 aabbExtents;
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

// Storage for the resulting frustums and AABBs.
layout(std140, binding = 0) restrict writeonly buffer AABBandFrustum
{
  TileData frustumsAABBs[];
};

shared uint minDepthUint;
shared uint maxDepthUint;

void main()
{
  const uint lInvoke = uint(gl_LocalInvocationIndex);

  if (lInvoke == 0)
  {
    minDepthUint = 0xFFFFFFFF;
    maxDepthUint = 0;
  }
  barrier();

  // Each thread fetches its depth and adds it to the shared cache for a min and max depth reduction.
  vec2 screenSize = vec2(textureSize(gDepth, 0).xy);
  vec2 gBufferUVs = (vec2(gl_GlobalInvocationID.xy) + 0.5.xx) / screenSize;

  float depth = textureLod(gDepth, gBufferUVs, 0.0).r;
  
  // Convert depth to uint so we can do atomic min and max comparisons between the threads
  uint depthUint = floatBitsToUint(depth);
  atomicMin(minDepthUint, depthUint);
  atomicMax(maxDepthUint, depthUint);
  barrier();

  // Only need one thread now to compute and write the frustum planes and AABBs.
  // Rest can safely return.
  if (lInvoke > 0)
    return;

  const ivec2 totalTiles = ivec2(gl_NumWorkGroups.xy); // Same as the total number of tiles.
  const ivec2 currentTile = ivec2(gl_WorkGroupID.xy); // The current tile.
  const uint tileOffset = totalTiles.x * currentTile.y + currentTile.x; // Offset into the tile buffer.

  vec2 nOffset = (2.0 * vec2(currentTile)) / vec2(totalTiles);               // Left
  vec2 pOffset = (2.0 * vec2(currentTile + ivec2(1, 1))) / vec2(totalTiles); // Right

  // Convert the min and max across the entire tile back to float
  float minDepth = uintBitsToFloat(minDepthUint);
  float lMinDepth = (0.5 * u_projMatrix[3][2]) / (minDepth + 0.5 * u_projMatrix[2][2] - 0.5);
  float maxDepth = uintBitsToFloat(maxDepthUint);
  float lMaxDepth = (0.5 * u_projMatrix[3][2]) / (maxDepth + 0.5 * u_projMatrix[2][2] - 0.5);

  TileData data;
  //----------------------------------------------------------------------------
  // Frustum calculations.
  //----------------------------------------------------------------------------
  data.frustumPlanes[0] = vec4( 1.0,  0.0,  0.0,  1.0 - nOffset.x); // Left
  data.frustumPlanes[1] = vec4(-1.0,  0.0,  0.0, -1.0 + pOffset.x); // Right
  data.frustumPlanes[2] = vec4( 0.0,  1.0,  0.0,  1.0 - nOffset.y); // Bottom
  data.frustumPlanes[3] = vec4( 0.0, -1.0,  0.0, -1.0 + pOffset.y); // Top
  data.frustumPlanes[4] = vec4( 0.0,  0.0, -1.0, -lMinDepth); // Near
  data.frustumPlanes[5] = vec4( 0.0,  0.0,  1.0,  lMaxDepth); // Far

  // Compute the first four worldspace planes.
  data.frustumPlanes[0] *= u_projMatrix * u_viewMatrix;
  data.frustumPlanes[0] /= length(data.frustumPlanes[0].xyz);
  data.frustumPlanes[1] *= u_projMatrix * u_viewMatrix;
  data.frustumPlanes[1] /= length(data.frustumPlanes[1].xyz);
  data.frustumPlanes[2] *= u_projMatrix * u_viewMatrix;
  data.frustumPlanes[2] /= length(data.frustumPlanes[2].xyz);
  data.frustumPlanes[3] *= u_projMatrix * u_viewMatrix;
  data.frustumPlanes[3] /= length(data.frustumPlanes[3].xyz);

  // Transform the depth planes.
  data.frustumPlanes[4] *= u_viewMatrix;
  data.frustumPlanes[4] /= length(data.frustumPlanes[4].xyz);
  data.frustumPlanes[5] *= u_viewMatrix;
  data.frustumPlanes[5] /= length(data.frustumPlanes[5].xyz);
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
  // AABB calculations.
  //----------------------------------------------------------------------------
  // Compute 8 corners.
  vec4 points[8];
  // Near.
  points[0] = vec4(2.0 * vec3(0.0, 0.0, minDepth) - 1.0.xxx, 1.0); // Bottom-left.
  points[1] = vec4(2.0 * vec3(1.0, 0.0, minDepth) - 1.0.xxx, 1.0); // Bottom-right.
  points[2] = vec4(2.0 * vec3(0.0, 1.0, minDepth) - 1.0.xxx, 1.0); // Top-left.
  points[3] = vec4(2.0 * vec3(1.0, 1.0, minDepth) - 1.0.xxx, 1.0); // Top-right.
  // Far.
  points[4] = vec4(2.0 * vec3(0.0, 0.0, maxDepth) - 1.0.xxx, 1.0); // Bottom-left.
  points[5] = vec4(2.0 * vec3(1.0, 0.0, maxDepth) - 1.0.xxx, 1.0); // Bottom-right.
  points[6] = vec4(2.0 * vec3(0.0, 1.0, maxDepth) - 1.0.xxx, 1.0); // Top-left.
  points[7] = vec4(2.0 * vec3(1.0, 1.0, maxDepth) - 1.0.xxx, 1.0); // Top-right.
  // NDC to world space.
  points[0] = u_invViewProjMatrix * points[0];
  points[0] /= points[0].w;
  points[1] = u_invViewProjMatrix * points[1];
  points[1] /= points[1].w;
  points[2] = u_invViewProjMatrix * points[2];
  points[2] /= points[2].w;
  points[3] = u_invViewProjMatrix * points[3];
  points[3] /= points[3].w;
  points[4] = u_invViewProjMatrix * points[4];
  points[4] /= points[4].w;
  points[5] = u_invViewProjMatrix * points[5];
  points[5] /= points[5].w;
  points[6] = u_invViewProjMatrix * points[6];
  points[6] /= points[6].w;
  points[7] = u_invViewProjMatrix * points[7];
  points[7] /= points[7].w;

  // Find the min and max.
  vec3 minCorner = points[0].xyz;
  vec3 maxCorner = points[0].xyz;
  minCorner = min(minCorner, points[1].xyz);
  maxCorner = max(maxCorner, points[1].xyz);
  minCorner = min(minCorner, points[2].xyz);
  maxCorner = max(maxCorner, points[2].xyz);
  minCorner = min(minCorner, points[3].xyz);
  maxCorner = max(maxCorner, points[3].xyz);
  minCorner = min(minCorner, points[4].xyz);
  maxCorner = max(maxCorner, points[4].xyz);
  minCorner = min(minCorner, points[5].xyz);
  maxCorner = max(maxCorner, points[5].xyz);
  minCorner = min(minCorner, points[6].xyz);
  maxCorner = max(maxCorner, points[6].xyz);
  minCorner = min(minCorner, points[7].xyz);
  maxCorner = max(maxCorner, points[7].xyz);

  // Compute the AABB.
  data.aabbCenter = vec4(0.5 * (maxCorner + minCorner), 1.0);
  data.aabbExtents = vec4(0.5 * (maxCorner - minCorner), 1.0);
  //----------------------------------------------------------------------------

  // Write to the SSBO.
  frustumsAABBs[tileOffset] = data;
}
