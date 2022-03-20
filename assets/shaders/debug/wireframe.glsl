#type common
#version 460 core
/*
 * A wireframe shader for debugging and visualizing objects.
 */

struct WireframeData
{
  mat4 transform;
  vec4 colour;
};

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

layout(std140, binding = 0) readonly buffer ModelBlock
{
  WireframeData u_data[];
};

#type vertex
layout(location = 0) in vec4 vPosition;

// Vertex properties for shading.
out VERT_OUT
{
  vec4 fColour;
} vertOut;

void main()
{
  const int index = gl_InstanceID;
  const mat4 modelMatrix = u_data[index].transform;

  gl_Position = u_projMatrix * u_viewMatrix * modelMatrix * vPosition;

  vertOut.fColour = u_data[index].colour;
}

#type fragment
layout(location = 0) out vec4 gColour;

in VERT_OUT
{
	vec4 fColour;
} fragIn;

void main()
{
  gColour = fragIn.fColour;
}
