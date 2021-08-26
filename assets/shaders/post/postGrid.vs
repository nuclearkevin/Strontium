#version 440

layout (location = 0) in vec2 vPosition;

// The post processing properties.
layout(std140, binding = 0) uniform PostProcessBlock
{
  mat4 u_invViewProj;
  mat4 u_viewProj;
  vec4 u_camPosScreenSize; // Camera position (x, y, z) and the screen width (w).
  vec4 u_screenSizeGammaBloom;  // Screen height (x), gamma (y) and bloom intensity (z). w is unused.
  ivec4 u_postProcessingPasses;
};

// Vertex properties for shading.
out VERT_OUT
{
  vec3 fNearPoint;
  vec3 fFarPoint;
} vertOut;

vec3 unProject(vec3 position, mat4 invVP);

void main()
{
  vertOut.fNearPoint = unProject(vec3(vPosition.x, vPosition.y, 0.0), u_invViewProj);
  vertOut.fFarPoint = unProject(vec3(vPosition.x, vPosition.y, 1.0), u_invViewProj);
  gl_Position = vec4(vPosition, 0.0, 1.0);
}

vec3 unProject(vec3 position, mat4 invVP)
{
  vec4 temp = invVP * vec4(position, 1.0);
  return temp.xyz / temp.w;
}
