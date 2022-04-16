#type compute
#version 460 core
/*
 * A compute shader to compute physically-based screenspace godrays. Based off
 * of:
 * https://www.alexandre-pestana.com/volumetric-lights/
*/

layout(local_size_x = 8, local_size_y = 8) in;

#define PI 3.141592654

#define NUM_CASCADES 4

#define EPSILON 1e-6

// TODO: Get density from a volume texture instead.
struct ScatteringParams
{
  vec4 mieScat; //  Mie scattering base (x, y, z) and density (w).
  vec4 mieAbs; //  Mie absorption base (x, y, z) and density (w).
  vec4 lightDirMiePhase; // Light direction (x, y, z) and the Mie phase (w).
  vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
};

// The output texture for the godrays.
layout(rgba16f, binding = 0) restrict writeonly uniform image2D godrayOut;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D noise;
// TODO: Texture arrays.
layout(binding = 2) uniform sampler2D cascadeMaps[NUM_CASCADES];

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

layout(std140, binding = 1) uniform GodrayBlock
{
  ScatteringParams u_params;
  vec4 u_godrayParams; // Blur direction (x, y), number of steps (z).
};

layout(std140, binding = 2) uniform CascadedShadowBlock
{
  mat4 u_lightVP[NUM_CASCADES];
  vec4 u_cascadeData[NUM_CASCADES]; // Cascade split distance (x). y, z and w are unused.
  vec4 u_shadowParams; // The constant bias (x), normal bias (y), the minimum PCF radius (z) and the cascade blend fraction (w).
};

vec4 computeGodrays(ivec2 coords, ivec2 outImageSize, sampler2D depthTex, ScatteringParams params);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  ivec2 outImageSize = ivec2(imageSize(godrayOut).xy);

  // Quit early for threads that aren't in bounds of the screen.
  if (any(greaterThanEqual(invoke, imageSize(godrayOut).xy)))
    return;

  vec4 result = computeGodrays(invoke, outImageSize, gDepth, u_params);

  imageStore(godrayOut, invoke, max(result, 0.0.xxxx));
}

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  return texture(noise, (vec2(coords) + 0.5.xx) / vec2(textureSize(noise, 0).xy));
}

// Decodes the worldspace position of the fragment from depth.
vec3 decodePosition(ivec2 coords, ivec2 outImageSize, sampler2D depthMap, mat4 invVP)
{
  vec2 texCoords = (vec2(coords) + 0.5.xx) / vec2(outImageSize);
  float depth = textureLod(depthMap, texCoords, 1.0).r;
  vec3 clipCoords = 2.0 * vec3(texCoords, depth) - 1.0.xxx;
  vec4 temp = invVP * vec4(clipCoords, 1.0);
  return temp.xyz / temp.w;
}

float getMiePhase(float cosTheta, float g)
{
  const float scale = 3.0 / (8.0 * PI);

  float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
  float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
}

float cascadedShadow(vec3 samplePos)
{
  vec4 clipSpacePos = u_viewMatrix * vec4(samplePos, 1.0);
  float shadowFactor = 1.0;
  for (uint i = 0; i < NUM_CASCADES; i++)
  {
    if (-clipSpacePos.z <= (u_cascadeData[i].x))
    {
      vec4 lightClipPos = u_lightVP[i] * vec4(samplePos, 1.0);
      vec3 projCoords = lightClipPos.xyz / lightClipPos.w;
      projCoords = 0.5 * projCoords + 0.5;

      shadowFactor = float(texture(cascadeMaps[i], projCoords.xy).r >= projCoords.z);
      break;
    }
  }

  return shadowFactor;
}

vec4 computeGodrays(ivec2 coords, ivec2 outImageSize, sampler2D depthTex, ScatteringParams params)
{
  const uint numSteps = uint(u_godrayParams.z);

  vec3 endPos = decodePosition(coords, outImageSize, depthTex, u_invViewProjMatrix);
  float rayT = length(endPos - u_camPosition) - EPSILON;
  float dt = rayT / max(float(numSteps) - 1.0, 1.0);
  vec3 rayDir = (endPos - u_camPosition) / rayT;
  vec3 startPos = u_camPosition + rayDir * dt * sampleDither(coords).r;
  rayT = length(endPos - startPos) - EPSILON;
  dt = rayT / max(float(numSteps) - 1.0, 1.0);

  float phase = getMiePhase(dot(rayDir, params.lightDirMiePhase.xyz), params.lightDirMiePhase.w);
  vec3 mieScattering = params.mieScat.xyz * params.mieScat.w;
  vec3 mieAbsorption = params.mieAbs.xyz * params.mieAbs.w;

  vec3 extinction = mieAbsorption + mieScattering;
  vec3 voxelAlbedo = mieScattering / extinction;

  vec3 luminance = 0.0.xxx;
  vec3 transmittance = 1.0.xxx;
  float t = 0.0;
  for (uint i = 0; i < numSteps; i++)
  {
    vec3 samplePos = startPos + t * rayDir;

    float visibility = cascadedShadow(samplePos);

    vec3 lScatt = voxelAlbedo * phase * visibility;
    vec3 sampleTransmittance = exp(-dt * extinction);
    vec3 scatteringIntegral = (lScatt - lScatt * sampleTransmittance) / extinction;

    luminance += scatteringIntegral * transmittance;
    transmittance *= sampleTransmittance;
    t += dt;
  }

  luminance *= params.lightColourIntensity.xyz * params.lightColourIntensity.w;

  return vec4(luminance, dot(vec3(1.0 / 3.0), transmittance));
}
