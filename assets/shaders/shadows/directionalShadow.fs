#version 440

#define WARP 44.0

layout(location = 0) out vec4 fragColour;

void main()
{
  float depth = gl_FragCoord.z;
  float dzdx = dFdx(depth);
  float dzdy = dFdy(depth);

  float posMom1 = exp(WARP * depth);
  float negMom1 = -1.0 * exp(-1.0 * WARP * depth);

  float posdFdx = WARP * posMom1 * dzdx;
  float posdFdy = WARP * posMom1 * dzdy;
  float posMom2 = posMom1 * posMom1 + (0.25 * (posdFdx * posdFdx + posdFdy * posdFdy));

  float negdFdx = -1.0 * WARP * negMom1 * dzdx;
  float negdFdy = -1.0 * WARP * negMom1 * dzdy;
  float negMom2 = negMom1 * negMom1 + (0.25 * (negdFdx * negdFdx + negdFdy * negdFdy));
  
  fragColour = vec4(posMom1, posMom2, negMom1, negMom2);
}
