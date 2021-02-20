#version 440
/*
 * The vertex shader for the light sources.
 */

layout (location = 0) in vec4 vPosition;

uniform mat4 mVP;

void main()
{
  gl_Position = mVP * vPosition;
}
