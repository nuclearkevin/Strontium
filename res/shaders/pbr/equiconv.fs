#version 440
/*
 * A fragment shader to convert an equirectangular map to a cube map.
 */

#define invAtan vec2(0.1591, 0.3183)

layout(location = 0) out vec4 fragColour;

in vec3 fPosition;

uniform sampler2D equirectangularMap;

vec2 SampleSphericalMap(vec3 v)
{
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= invAtan;
  uv += 0.5;
  return uv;
}

void main()
{
  vec2 uv = SampleSphericalMap(normalize(fPosition));
  vec3 color = texture(equirectangularMap, uv).rgb;

  fragColour = vec4(color, 1.0);
}
