#version 440

layout (location = 0) in vec4 vPosition;

layout(std140, binding = 2) uniform ModelBlock
{
  mat4 u_modelMatrix;
};

layout(std140, binding = 6) uniform LightSpaceBlock
{
  mat4 u_lightViewProj;
};

void main()
{
  gl_Position = u_lightViewProj * u_modelMatrix * vPosition;
}
