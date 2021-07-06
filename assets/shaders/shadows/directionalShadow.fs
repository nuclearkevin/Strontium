#version 440

layout(location = 0) out vec4 moments;

void main()
{
  moments = vec4(vec2(gl_FragCoord.z, gl_FragCoord.z * gl_FragCoord.z), 1.0, 1.0);
}
