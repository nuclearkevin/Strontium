#version 440
/*
 * A vertex shader for the geometry pass in deferred rendering.
 */

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColour;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec3 vTangent;
layout (location = 5) in vec3 vBitangent;

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

// Vertex properties for shading.
out VERT_OUT
{
  vec3 fNormal;
 	vec3 fPosition;
 	vec3 fColour;
  vec2 fTexCoords;
 	mat3 fTBN;
} vertOut;

void main()
{
  // Tangent to world matrix calculation.
 	vec3 T = normalize(vec3(u_modelMatrix * vec4(vTangent, 0.0)));
 	vec3 N = normalize(vec3(u_modelMatrix * vec4(vNormal, 0.0)));
 	T = normalize(T - dot(T, N) * N);
 	vec3 B = cross(N, T);

 	gl_Position = u_projMatrix * u_viewMatrix * u_modelMatrix * vPosition;
  vertOut.fPosition = (u_modelMatrix * vPosition).xyz;
 	vertOut.fNormal = N;
 	vertOut.fColour = vColour;
 	vertOut.fTexCoords = vTexCoord;
 	vertOut.fTBN = mat3(T, B, N);
}
