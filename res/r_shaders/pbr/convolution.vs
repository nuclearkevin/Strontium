#version 440
/*
 * A vertex shader to pass parameters through to the convolution fragment shader.
*/

layout (location = 0) in vec4 vPosition;

out vec3 fPosition;

uniform mat4 vP;

void main()
{
  fPosition = vPosition.xyz;
  gl_Position = vP * vPosition;
}
