#type compute
#version 460 core
/*
 * A compute shader to precompute the frustums and AABBs used in tiled
 * deferred rendering.
 */

#define GROUP_SIZE 16

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE) in;

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

shared float minDepth[GROUP_SIZE * GROUP_SIZE];
shared float maxDepth[GROUP_SIZE * GROUP_SIZE];

void main()
{
  const uint lInvoke = uint(gl_LocalInvocationIndex);

  // Each thread fetches its depth and adds it to the shared cache for a min and max depth reduction.
  vec2 screenSize = vec2(textureSize(gDepth, 0).xy);
  vec2 gBufferUVs = (vec2(gl_GlobalInvocationID.xy) + 0.5.xx) / screenSize;

  float depth = textureLod(gDepth, gBufferUVs, 0.0).r;
  minDepth[lInvoke] = depth;
  maxDepth[lInvoke] = depth;
  barrier();

  // Parallel min/max reduction for two arrays of 256 elements.
  if (lInvoke < 128)
  {
    minDepth[lInvoke] = min(minDepth[lInvoke], minDepth[lInvoke + 128]);
    maxDepth[lInvoke] = max(maxDepth[lInvoke], maxDepth[lInvoke + 128]);
  }
  barrier();

  if (lInvoke < 64)
  {
    minDepth[lInvoke] = min(minDepth[lInvoke], minDepth[lInvoke + 64]);
    maxDepth[lInvoke] = max(maxDepth[lInvoke], maxDepth[lInvoke + 64]);
  }
  barrier();

  if (lInvoke < 32)
  {
    minDepth[lInvoke] = min(minDepth[lInvoke], minDepth[lInvoke + 32]);
    maxDepth[lInvoke] = max(maxDepth[lInvoke], maxDepth[lInvoke + 32]);
  }
  barrier();

  if (lInvoke < 16)
  {
    minDepth[lInvoke] = min(minDepth[lInvoke], minDepth[lInvoke + 16]);
    maxDepth[lInvoke] = max(maxDepth[lInvoke], maxDepth[lInvoke + 16]);
  }
  barrier();

  if (lInvoke < 8)
  {
    minDepth[lInvoke] = min(minDepth[lInvoke], minDepth[lInvoke + 8]);
    maxDepth[lInvoke] = max(maxDepth[lInvoke], maxDepth[lInvoke + 8]);
  }
  barrier();

  if (lInvoke < 4)
  {
    minDepth[lInvoke] = min(minDepth[lInvoke], minDepth[lInvoke + 4]);
    maxDepth[lInvoke] = max(maxDepth[lInvoke], maxDepth[lInvoke + 4]);
  }
  barrier();

  if (lInvoke < 2)
  {
    minDepth[lInvoke] = min(minDepth[lInvoke], minDepth[lInvoke + 2]);
    maxDepth[lInvoke] = max(maxDepth[lInvoke], maxDepth[lInvoke + 2]);
  }
  barrier();

  if (lInvoke < 1)
  {
    minDepth[lInvoke] = min(minDepth[lInvoke], minDepth[lInvoke + 1]);
    maxDepth[lInvoke] = max(maxDepth[lInvoke], maxDepth[lInvoke + 1]);
  }
  barrier();

  // Only need one thread now to compute and write the frustum planes and AABBs.
  // Rest can safely return.
  if (lInvoke > 0)
    return;

  ivec2 numGroups = ivec2(gl_NumWorkGroups.xy);
  ivec2 groupID = ivec2(gl_WorkGroupID.xy);

  vec2 nOffset = (2.0 * vec2(groupID)) / vec2(numGroups);               // Left
  vec2 pOffset = (2.0 * vec2(groupID + ivec2(1, 1))) / vec2(numGroups); // Right

  TileData data;
  //----------------------------------------------------------------------------
  // Frustum calculations.
  //----------------------------------------------------------------------------
  data.frustumPlanes[0] = vec4( 1.0,  0.0,  0.0,  1.0 - nOffset.x); // Left
  data.frustumPlanes[1] = vec4(-1.0,  0.0,  0.0, -1.0 + pOffset.x); // Right
  data.frustumPlanes[2] = vec4( 0.0,  1.0,  0.0,  1.0 - nOffset.y); // Bottom
  data.frustumPlanes[3] = vec4( 0.0, -1.0,  0.0, -1.0 + pOffset.y); // Top
  data.frustumPlanes[4] = vec4( 0.0,  0.0, -1.0, -minDepth[0]); // Near
  data.frustumPlanes[5] = vec4( 0.0,  0.0,  1.0,  maxDepth[0]); // Far

  // Compute the first four worldspace planes.
  for (uint i = 0; i < 4; i++)
  {
    data.frustumPlanes[i] *= u_projMatrix * u_viewMatrix;
    data.frustumPlanes[i] /= length(data.frustumPlanes[i].xyz);
  }

  // Have to treat the near and far planes separately.
  for (uint i = 4; i < 6; i++)
  {
    data.frustumPlanes[i] *= u_viewMatrix;
    data.frustumPlanes[i] /= length(data.frustumPlanes[i].xyz);
  }
  //----------------------------------------------------------------------------
  // AABB calculations.
  //----------------------------------------------------------------------------
  vec4 aabbMin = vec4(nOffset, minDepth[0], 1.0);
  vec4 aabbMax = vec4(pOffset, maxDepth[0], 1.0);
  aabbMin *= u_invViewProjMatrix;
  aabbMax *= u_invViewProjMatrix;
  aabbMin.xyz /= aabbMin.w;
  aabbMax.xyz /= aabbMax.w;

  aabbMin = u_viewMatrix * vec4(aabbMin.xyz, 1.0);
  aabbMax = u_viewMatrix * vec4(aabbMax.xyz, 1.0);

  data.aabbCenter = (aabbMin + aabbMax) / 2.0;
  data.aabbExtents = max(abs(aabbMin), abs(aabbMax));
  //----------------------------------------------------------------------------

  // Write to the SSBO.
  frustumsAABBs[numGroups.x * groupID.y + groupID.x] = data;
}
