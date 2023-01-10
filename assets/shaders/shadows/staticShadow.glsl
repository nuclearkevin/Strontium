#type common
#version 460 core
/*
 * A directional light shadow shader for static meshes.
 */

struct VertexData
{
  vec4 normal;
  vec4 tangent;
  vec4 position; // Uncompressed position (x, y, z). w is padding.
  vec4 boneWeights; // Uncompressed bone weights.
  ivec4 boneIDs; // Bone IDs.
  vec4 texCoord; // UV coordinates (x, y). z and w are padding.
};

#type vertex
// The view-projection matrix for the light.
layout(std140, binding = 0) uniform LightSpaceBlock
{
  mat4 u_lightViewProj;
};

layout(std140, binding = 0) readonly buffer VertexBuffer
{
  VertexData v_vertices[];
};

layout(std430, binding = 1) readonly buffer IndexBuffer
{
  uint v_indices[];
};

// The model matrix.
layout(std140, binding = 2) readonly buffer TransformBlock
{
  mat4 u_transforms[];
};

// The draw to transform matrix mapping.
layout(std430, binding = 4) readonly buffer DrawTransformMapping
{
  uint u_mapping[];
};

void main()
{
  const uint vIndex = v_indices[gl_VertexID];
  const VertexData vertex = v_vertices[vIndex];

  // Compute the index of this draw into the global buffer.
  const uint instance = uint(gl_InstanceID) + u_mapping[uint(gl_DrawID)];

  gl_Position = u_lightViewProj * u_transforms[instance] * vec4(vertex.position.xyz, 1.0);
}

#type fragment
void main()
{ }
