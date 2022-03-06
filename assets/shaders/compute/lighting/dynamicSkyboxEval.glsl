#type compute
#version 460 core
/*
 * A compute shader for evaluating cubemap or LUT-based skyboxes.
 */

#define MAX_MIP 4.0

#define MAX_NUM_ATMOSPHERES 8
#define TWO_PI 6.283185308
#define PI 3.141592654
#define PI_OVER_TWO 1.570796327

layout(local_size_x = 8, local_size_y = 8) in;

// Atmospheric scattering coefficients are expressed per unit km.
struct ScatteringParams
{
  vec4 rayleighScat; //  Rayleigh scattering base (x, y, z) and height falloff (w).
  vec4 rayleighAbs; //  Rayleigh absorption base (x, y, z) and height falloff (w).
  vec4 mieScat; //  Mie scattering base (x, y, z) and height falloff (w).
  vec4 mieAbs; //  Mie absorption base (x, y, z) and height falloff (w).
  vec4 ozoneAbs; //  Ozone absorption base (x, y, z) and scale (w).
};

// Radii are expressed in megameters (MM).
struct AtmosphereParams
{
  ScatteringParams sParams;
  vec4 planetAlbedoRadius; // Planet albedo (x, y, z) and radius.
  vec4 sunDirAtmRadius; // Sun direction (x, y, z) and atmosphere radius (w).
  vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  vec4 viewPos; // View position (x, y, z). w is unused.
};

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2DArray transmittanceLUTs;
layout(binding = 2) uniform sampler2DArray dynamicSkyLUTs;

layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y). z and w are unused.
};

layout(std140, binding = 1) uniform HillaireParams
{
  AtmosphereParams u_atmoParams[MAX_NUM_ATMOSPHERES];
};

layout(std140, binding = 2) uniform SkyboxParams
{
  vec4 u_params; // Atmosphere index (x) and the sun solid angle (y), sky intensity (z).
};

// Quick and dirty ray-sphere intersection test.
float rayIntersectSphere(vec3 ro, vec3 rd, float rad);

// Decodes the worldspace position of the fragment from depth.
vec3 decodePosition(vec2 texCoords, float depth, mat4 invMVP);

// Sample the skyview LUT from Hillaire2020.
vec3 sampleSkyViewLUT(float atmIndex, sampler2DArray tex, vec3 viewPos, vec3 viewDir,
                      vec3 sunDir, float groundRadiusMM);
// Sample the transmittance LUT from Hillaire2020.
vec3 sampleHLUT(float atmIndex, sampler2DArray tex, vec3 pos, vec3 sunDir,
                float groundRadiusMM, float atmosphereRadiusMM);

// Compute the sun disk for either dynamic sky model.
vec3 computeSunLuminance(vec3 e, vec3 s, vec3 colour, float size, float intensity);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(textureSize(gDepth, 0).xy);

  // Skybox should only be drawn when the depth of a fragment is 1.0, early out
  // if that is not the case.
  float depth = textureLod(gDepth, gBufferUVs, 0.0).r;
  if (depth != 1.0)
    return;

  // Compute the ray direction.
  vec3 position = decodePosition(gBufferUVs, depth, u_invViewProjMatrix);
  vec3 viewDir = normalize(position - u_camPosition);

  const int atmoIndex = int(u_params.x);
  const AtmosphereParams params = u_atmoParams[atmoIndex];

  float groundRadiusMM = params.planetAlbedoRadius.w;
  float atmosphereRadiusMM = params.sunDirAtmRadius.w;

  vec3 sunDir = normalize(-params.sunDirAtmRadius.xyz);
  vec3 sunColour = params.lightColourIntensity.xyz;
  float sunIntensity = params.lightColourIntensity.w;
  float sunSize = u_params.y;
  vec3 viewPos = vec3(params.viewPos.xyz);

  // Compute the sun disk.
  float intGround = float(rayIntersectSphere(viewPos, viewDir, groundRadiusMM) < 0.0);
  vec3 colour = computeSunLuminance(viewDir, sunDir, sunColour, sunSize, sunIntensity);
  vec3 groundTrans = sampleHLUT(float(atmoIndex), transmittanceLUTs, viewPos,
                                sunDir, groundRadiusMM, atmosphereRadiusMM);
  vec3 spaceTrans = sampleHLUT(float(atmoIndex), transmittanceLUTs,
                               vec3(0.0, groundRadiusMM, 0.0),
                               vec3(0.0, 1.0, 0.0), groundRadiusMM,
                               atmosphereRadiusMM);
  colour *= intGround * groundTrans / spaceTrans;

  // Add on the sky contribution.
  colour += sampleSkyViewLUT(float(atmoIndex), dynamicSkyLUTs, viewPos, viewDir,
                             sunDir, groundRadiusMM);

  // Load and store the radiance.
  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;
  totalRadiance += u_params.z * colour;
  imageStore(lightingBuffer, invoke, vec4(totalRadiance, 1.0));
}

vec3 decodePosition(vec2 texCoords, float depth, mat4 invMVP)
{
  vec3 clipCoords = 2.0 * vec3(texCoords, depth) - 1.0.xxx;
  vec4 temp = invMVP * vec4(clipCoords, 1.0);
  return temp.xyz / temp.w;
}

float rayIntersectSphere(vec3 ro, vec3 rd, float rad)
{
  float b = dot(ro, rd);
  float c = dot(ro, ro) - rad * rad;
  if (c > 0.0f && b > 0.0)
    return -1.0;

  float discr = b * b - c;
  if (discr < 0.0)
    return -1.0;

  // Special case: inside sphere, use far discriminant
  if (discr > b * b)
    return (-b + sqrt(discr));

  return -b - sqrt(discr);
}

float safeACos(float x)
{
  return acos(clamp(x, -1.0, 1.0));
}

vec3 sampleSkyViewLUT(float atmIndex, sampler2DArray tex, vec3 viewPos, vec3 viewDir,
                      vec3 sunDir, float groundRadiusMM)
{
  float height = length(viewPos);
  vec3 up = viewPos / height;

  float horizonAngle = safeACos(sqrt(height * height - groundRadiusMM * groundRadiusMM) / height);
  float altitudeAngle = horizonAngle - acos(dot(viewDir, up));

  vec3 right = cross(sunDir, up);
  vec3 forward = cross(up, right);

  vec3 projectedDir = normalize(viewDir - up * (dot(viewDir, up)));
  float sinTheta = dot(projectedDir, right);
  float cosTheta = dot(projectedDir, forward);
  float azimuthAngle = atan(sinTheta, cosTheta) + PI;

  float u = azimuthAngle / (TWO_PI);
  float v = 0.5 + 0.5 * sign(altitudeAngle) * sqrt(abs(altitudeAngle) / PI_OVER_TWO);

  return textureLod(tex, vec3(u, v, atmIndex), 0.0).rgb;
}

vec3 sampleHLUT(float atmIndex, sampler2DArray tex, vec3 pos, vec3 sunDir,
                float groundRadiusMM, float atmosphereRadiusMM)
{
  float height = length(pos);
  vec3 up = pos / height;

  float sunCosZenithAngle = dot(sunDir, up);

  float u = clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0);
  float v = max(0.0, min(1.0, (height - groundRadiusMM) / (atmosphereRadiusMM - groundRadiusMM)));

  return textureLod(tex, vec3(u, v, atmIndex), 0.0).rgb;
}

vec3 computeSunLuminance(vec3 e, vec3 s, vec3 colour, float size, float intensity)
{
  float sunSolidAngle = size * PI * 0.005555556; // 1.0 / 180.0
  float minSunCosTheta = cos(sunSolidAngle);

  float cosTheta = dot(s, e);
  float angle = safeACos(cosTheta);
  float radiusRatio = angle / sunSolidAngle;
  float limbDarkening = sqrt(clamp(1.0 - radiusRatio * radiusRatio, 0.0001, 1.0));

  float comp = float(cosTheta >= minSunCosTheta);
  return colour * intensity * comp * limbDarkening;
}
