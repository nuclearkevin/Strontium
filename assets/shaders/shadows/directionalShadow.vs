#version 440

layout (location = 0) in vec4 vPosition;

uniform mat4 model;
uniform mat4 lightVP;

void main()
{
  gl_Position = lightVP * model * vPosition;
}
