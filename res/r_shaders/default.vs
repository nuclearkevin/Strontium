#version 440
/*
 *  Vertex shader to experiment with structs.
 */

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColour;

uniform mat4 mVP;
uniform mat3 normalMat;
uniform mat4 model;

out vec3 fNormal;
out vec4 fPosition;
out vec3 fColour;

void main()
{

	gl_Position = mVP * vPosition;
  fPosition = model * vPosition;
	fNormal = normalMat * vNormal;
	fColour = vColour;
}
