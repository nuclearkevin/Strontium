#version 440

uniform float shadowTuning = 1.0;

layout(location = 0) out vec4 fragColour;

void main()
{
  fragColour = vec4(vec3(exp(shadowTuning * gl_FragCoord.z)), 1.0);
}
