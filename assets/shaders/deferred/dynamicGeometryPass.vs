#version 440
/*
 * A vertex shader for a skinned geometry pass in a deferred renderer.
 */

#define MAX_BONES_PER_MODEL 512

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in vec3 vBitangent;
layout (location = 5) in vec4 vBoneWeight;
layout (location = 6) in ivec4 vBoneID;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  vec3 u_camPosition;
};

layout(std140, binding = 2) uniform ModelBlock
{
  mat4 u_modelMatrix;
};

layout(std140, binding = 4) buffer BoneBlock
{
  mat4 u_boneMatrices[MAX_BONES_PER_MODEL];
};

// Vertex properties for shading.
out VERT_OUT
{
  vec3 fNormal;
 	vec3 fPosition;
  vec2 fTexCoords;
 	mat3 fTBN;
} vertOut;

void main()
{
  // Skinning calculations.
  mat4 skinMatrix = u_boneMatrices[vBoneID.x] * vBoneWeight.x;
  skinMatrix += u_boneMatrices[vBoneID.y] * vBoneWeight.y;
  skinMatrix += u_boneMatrices[vBoneID.z] * vBoneWeight.z;
  skinMatrix += u_boneMatrices[vBoneID.w] * vBoneWeight.w;

  skinMatrix = vBoneWeight.x > 0.0 ? skinMatrix : mat4(1.0);

  mat4 worldSpaceMatrix = u_modelMatrix * skinMatrix;

  // Tangent to world matrix calculation.
 	vec3 T = normalize(vec3(worldSpaceMatrix * vec4(vTangent, 0.0)));
 	vec3 N = normalize(vec3(worldSpaceMatrix * vec4(vNormal, 0.0)));
 	T = normalize(T - dot(T, N) * N);
 	vec3 B = cross(N, T);

 	gl_Position = u_projMatrix * u_viewMatrix * worldSpaceMatrix * vPosition;
  vertOut.fPosition = (worldSpaceMatrix * vPosition).xyz;
 	vertOut.fNormal = N;
 	vertOut.fTexCoords = vTexCoord;
 	vertOut.fTBN = mat3(T, B, N);
}
