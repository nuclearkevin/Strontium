#type common
#version 460 core
/*
 * A directional light shadow shader for static meshes. Exponentially-warped
 * variance shadowmaps.
 */
#define NUM_CASCADES 4

#type vertex
layout (location = 0) in vec4 vPosition;

// The view-projection matrix for the light.
layout(std140, binding = 0) uniform LightSpaceBlock
{
  mat4 u_lightViewProj;
};

// An index for fetching data from the transform and editor SSBOs.
layout(std140, binding = 1) uniform PerDrawBlock
{
  ivec4 u_drawData; // Transform ID (x). Y, z and w are unused.
};

// The model matrix.
layout(std140, binding = 0) readonly buffer TransformBlock
{
  mat4 u_transforms[];
};

void main()
{
  // Compute the index of this draw into the global buffer.
  const int index = gl_InstanceID + u_drawData.x;

  gl_Position = u_lightViewProj * u_transforms[index] * vPosition;
}

#type fragment

void main()
{ }
