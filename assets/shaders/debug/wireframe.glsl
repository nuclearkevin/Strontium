#type common
#version 460 core
/*
 * A wireframe shader for debugging and visualizing objects.
 */

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

layout(std140, binding = 1) uniform ColourBlock
{
  vec4 u_colour;
};

layout(std140, binding = 0) readonly buffer ModelBlock
{
  mat4 u_transform[];
};

#type vertex
layout(location = 0) in vec4 vPosition;

void main()
{
  const int index = gl_InstanceID;
  const mat4 modelMatrix = u_transform[index];

  gl_Position = u_projMatrix * u_viewMatrix * modelMatrix * vPosition;
}

#type fragment
layout(location = 0) out vec4 gColour;

void main()
{
  gColour = u_colour;
}
