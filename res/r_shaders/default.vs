#version 440
/*
 *  Default model vertex shader.
 */
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColour;
layout (location = 3) in vec2 vTexCoords;

uniform mat4 mVP;
uniform mat3 normalMat;
uniform mat4 model;

out VERT_OUT
{
	vec3 fNormal;
	vec3 fPosition;
	vec3 fColour;
  vec2 fTexCoords;
} vertOut;

void main()
{
	gl_Position = mVP * vPosition;
  vertOut.fPosition = (model * vPosition).xyz;
	vertOut.fNormal = normalMat * vNormal;
	vertOut.fColour = vColour;
	vertOut.fTexCoords = vTexCoords;
}
