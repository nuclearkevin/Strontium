#version 330 core
/*
 *  Simple vertex shader for example four
 */

in vec4 vPosition;
in vec3 vNormal;
uniform mat4 model;
uniform mat3 normalMat;
out vec3 normal;

void main() {

	gl_Position = model * vPosition;
	normal = normalMat* vNormal;
}