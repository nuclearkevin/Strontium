#version 440

in VERT_OUT
{
  vec3 fNearPoint;
  vec3 fFarPoint;
  mat4 fInvViewProj;
} fragIn;

uniform mat4 viewProj;
layout(binding = 0) uniform sampler2D gDepth;

layout(location = 1) out vec4 fragColour;

// Helper functions.
vec3 unProject(vec3 position, mat4 invVP);

// Compute the x-z grid.
vec4 grid(vec3 fragPos3D, float scale);

void main()
{
  float t = -fragIn.fNearPoint.y / (fragIn.fFarPoint.y - fragIn.fNearPoint.y);
  vec3 fragPos3D = fragIn.fNearPoint + t * (fragIn.fFarPoint - fragIn.fNearPoint);

  // Compute the depth of the current fragment [0, 1].
  vec4 fragClipPos = viewProj * vec4(fragPos3D, 1.0);
  float fragDepth = 0.5 * (fragClipPos.z / fragClipPos.w) + 0.5;
  // Fetch the scene depth.
  float sceneDepth = texture(gDepth, gl_FragCoord.xy / textureSize(gDepth, 0)).r;

  vec3 screenNear = unProject(vec3(0.0), fragIn.fInvViewProj);
  float falloff = max(1.5 - 0.2 * length(fragPos3D.xz - screenNear.xz), 0.0);

  fragColour = (grid(fragPos3D, 10.0) + grid(fragPos3D, 1.0)) * float(t > 0) * falloff;

  if (sceneDepth < fragDepth)
    fragColour = vec4(vec3(0.0), 1.0);
}

vec3 unProject(vec3 position, mat4 invVP)
{
  vec4 temp = invVP * vec4(position, 1.0);
  return temp.xyz / temp.w;
}

vec4 grid(vec3 fragPos3D, float scale)
{
  vec2 coord = fragPos3D.xz * scale;
  vec2 derivative = fwidth(coord);
  vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
  float line = min(grid.x, grid.y);

  float minimumz = min(derivative.y, 1);
  float minimumx = min(derivative.x, 1);

  float transparency = 1.0 - min(line, 1.0);
  transparency *= 0.2;
  vec4 color = vec4(transparency, transparency, transparency, 1.0);

  // Z axis.
  if (fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
      color.z = 1.0;
  // X axis.
  if (fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
      color.x = 1.0;

  return color;
}
