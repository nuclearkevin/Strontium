#version 330 core
/*
 *  Vertex shader to experiment with structs.
 */

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;

uniform mat4 model;
uniform mat3 normalMat;

out vec3 normal;

void main() {

	gl_Position = model * vPosition;
	normal = normalMat * vNormal;
}
