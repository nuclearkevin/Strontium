#version 440
/*
 *  Default model vertex shader.
 */
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColour;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec3 vTangent;
layout (location = 5) in vec3 vBitangent;

uniform mat4 mVP;
uniform mat3 normalMat;
uniform mat4 model;

// Vertex properties for shading.
out VERT_OUT
{
	vec3 fNormal;
	vec3 fPosition;
	vec3 fColour;
  vec2 fTexCoords;
	mat3 fTBN;
} vertOut;

void main()
{
	// Tangent to world matrix calculation.
	vec3 T = normalize(vec3(model * vec4(vTangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(vNormal, 0.0)));
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	gl_Position = mVP * vPosition;
  vertOut.fPosition = (model * vPosition).xyz;
	vertOut.fNormal = N;
	vertOut.fColour = vColour;
	vertOut.fTexCoords = vTexCoord;
	vertOut.fTBN = mat3(T, B, N);
}
