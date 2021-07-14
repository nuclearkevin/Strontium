#version 440

#define WARP 44.0

layout(location = 0) out vec4 fragColour;

void main()
{
  float depth = gl_FragCoord.z;

  float posMom1 = exp(WARP * depth);
  float negMom1 = -1.0 * exp(-1.0 * WARP * depth);
  fragColour = vec4(posMom1, posMom1 * posMom1, negMom1, negMom1 * negMom1);
}
