#version 440 core
/*
 *  Vertex shader to experiment with structs.
 */

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColour;

uniform mat4 mVP;
uniform mat3 normalMat;
uniform mat4 model;

out vec3 normal;
out vec4 position;
out vec3 colour;

void main() {

	gl_Position = mVP * vPosition;
  position = model * vPosition;
	normal = normalMat * vNormal;
	colour = vColour;
}
