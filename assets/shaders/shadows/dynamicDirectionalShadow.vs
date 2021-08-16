#version 440

#define MAX_BONES_PER_MODEL 512

layout (location = 0) in vec4 vPosition;
layout (location = 5) in vec4 vBoneWeight;
layout (location = 6) in ivec4 vBoneID;

layout(std140, binding = 2) uniform ModelBlock
{
  mat4 u_modelMatrix;
};

layout(std140, binding = 6) uniform LightSpaceBlock
{
  mat4 u_lightViewProj;
};

layout(std140, binding = 4) buffer BoneBlock
{
  mat4 u_boneMatrices[MAX_BONES_PER_MODEL];
};

void main()
{
  // Skinning calculations.
  mat4 skinMatrix = u_boneMatrices[vBoneID.x] * vBoneWeight.x;
  skinMatrix += u_boneMatrices[vBoneID.y] * vBoneWeight.y;
  skinMatrix += u_boneMatrices[vBoneID.z] * vBoneWeight.z;
  skinMatrix += u_boneMatrices[vBoneID.w] * vBoneWeight.w;

  mat4 worldSpaceMatrix = u_modelMatrix * skinMatrix;

  gl_Position = u_lightViewProj * worldSpaceMatrix * vPosition;
}
