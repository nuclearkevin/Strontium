#version 440

layout (location = 0) in vec4 vPosition;

out VERT_OUT
{
  vec3 fTexCoords;
} vertOut;

uniform mat4 vP;

void main()
{
    vertOut.fTexCoords = vPosition.xyz;
    gl_Position = (vP * vPosition).xyww;
}
