#type common
#version 460 core
/*
 * A directional light shadow shader for dynamic meshes.
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
#define MAX_BONES_PER_MODEL 512

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

layout(std140, binding = 3) readonly buffer BoneBlock
{
  mat4 u_boneMatrices[MAX_BONES_PER_MODEL];
};

void main()
{
  const uint vIndex = v_indices[gl_VertexID];
  const VertexData vertex = v_vertices[vIndex];

  // Compute the index of this draw into the global buffer.
  const int instance = gl_InstanceID + u_drawData.x;

  // Fetch the transform from the global buffer.
  const mat4 modelMatrix = u_transforms[instance];

  // Skinning calculations.
  mat4 skinMatrix = vertex.boneIDs.x > -1 ? u_boneMatrices[vertex.boneIDs.x]
                                          * vertex.boneWeights.x
                                          : mat4(1.0);
  skinMatrix += u_boneMatrices[vertex.boneIDs.y] * vertex.boneWeights.y;
  skinMatrix += u_boneMatrices[vertex.boneIDs.z] * vertex.boneWeights.z;
  skinMatrix += u_boneMatrices[vertex.boneIDs.w] * vertex.boneWeights.w;

  mat4 worldSpaceMatrix = modelMatrix * skinMatrix;

  gl_Position = u_lightViewProj * worldSpaceMatrix * vec4(vertex.position.xyz, 1.0);
}


#type fragment
void main()
{ }
