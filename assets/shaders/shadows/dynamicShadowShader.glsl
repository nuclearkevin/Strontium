#type common
#version 460 core
/*
 * A directional light shadow shader for dynamic meshes. Exponentially-warped
 * variance shadowmaps.
 */
#define NUM_CASCADES 4

#type vertex
#define MAX_BONES_PER_MODEL 512

layout (location = 0) in vec4 vPosition;
layout (location = 5) in vec4 vBoneWeight;
layout (location = 6) in ivec4 vBoneID;

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

layout(std140, binding = 1) readonly buffer BoneBlock
{
  mat4 u_boneMatrices[MAX_BONES_PER_MODEL];
};

void main()
{
  // Compute the index of this draw into the global buffer.
  const int index = gl_InstanceID + u_drawData.x;

  // Fetch the transform from the global buffer.
  const mat4 modelMatrix = u_transforms[index];

  // Skinning calculations.
  mat4 skinMatrix = vBoneID.x > -1 ? u_boneMatrices[vBoneID.x] * vBoneWeight.x
                                   : mat4(1.0);
  skinMatrix += u_boneMatrices[vBoneID.y] * vBoneWeight.y;
  skinMatrix += u_boneMatrices[vBoneID.z] * vBoneWeight.z;
  skinMatrix += u_boneMatrices[vBoneID.w] * vBoneWeight.w;

  mat4 worldSpaceMatrix = modelMatrix * skinMatrix;

  gl_Position = u_lightViewProj * worldSpaceMatrix * vPosition;
}


#type fragment

void main()
{ }
