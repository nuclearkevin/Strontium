#type common
#version 460 core
/*
 * A line shader for debugging and visualizing objects.
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

#type vertex
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColour;

// Vertex properties for shading.
out VERT_OUT
{
  vec3 fColour;
} vertOut;

void main()
{
  gl_Position = u_projMatrix * u_viewMatrix * vec4(vPosition, 1.0);
  vertOut.fColour = vColour;
}

#type fragment
layout(location = 0) out vec4 gColour;

in VERT_OUT
{
  vec3 fColour;
} fragIn;

void main()
{
  gColour = vec4(fragIn.fColour, 1.0);
}
