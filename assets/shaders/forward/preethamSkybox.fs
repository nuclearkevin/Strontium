#version 440

/*
 * A generic forward pass Preetham skybox shader. Shares a buffer block with the
 * IBL skybox shader. Adapted from https://www.shadertoy.com/view/llSSDR.
*/

#define TWO_PI 6.283185308
#define PI 3.141592654
#define PI_OVER_TWO 1.570796327

layout(binding = 0) uniform sampler2D preethamLUT;

layout(std140, binding = 1) uniform SkyboxBlock
{
  vec4 u_lodDirection; // IBL lod (x), sun direction (y, z, w).
};

in VERT_OUT
{
  vec3 fTexCoords;
} fragIn;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

vec2 sampleHemisphericalMap(vec3 viewDir)
{
  float inclination = asin(viewDir.y);
  float azimuth = atan(viewDir.x, viewDir.z) + PI;

  float u = azimuth / TWO_PI;
  float v = 0.5 * (inclination / PI_OVER_TWO + 1.0);

  return vec2(u, v);
}

void main()
{
  vec3 viewDir = normalize(fragIn.fTexCoords);

  vec2 uv = sampleHemisphericalMap(viewDir);

  vec3 skyRadiance = texture(preethamLUT, uv).rgb;
  fragColour = vec4(skyRadiance, 1.0);
}
