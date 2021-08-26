#version 440

/*
 * A generic forward pass skybox shader.
*/

layout(location = 0) in vec4 vPosition;

out VERT_OUT
{
  vec3 fTexCoords;
} vertOut;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  vec3 u_camPosition;
};

void main()
{
  vertOut.fTexCoords = vPosition.xyz;

  mat4 newView = mat4(mat3(u_viewMatrix));
  gl_Position = (u_projMatrix * newView * vPosition).xyww;
}
