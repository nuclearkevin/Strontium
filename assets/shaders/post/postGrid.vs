#version 440

layout (location = 0) in vec2 vPosition;

uniform mat4 invViewProj;
uniform vec3 camPos;

// Vertex properties for shading.
out VERT_OUT
{
  vec3 fNearPoint;
  vec3 fFarPoint;
  mat4 fInvViewProj;
} vertOut;

vec3 unProject(vec3 position, mat4 invVP);

void main()
{
  vertOut.fNearPoint = unProject(vec3(vPosition.x, vPosition.y, 0.0), invViewProj);
  vertOut.fFarPoint = unProject(vec3(vPosition.x, vPosition.y, 1.0), invViewProj);
  vertOut.fInvViewProj = invViewProj;
  gl_Position = vec4(vPosition, 0.0, 1.0);
}

vec3 unProject(vec3 position, mat4 invVP)
{
  vec4 temp = invVP * vec4(position, 1.0);
  return temp.xyz / temp.w;
}
