#type compute
#version 460 core

#define PI 3.141592654
#define NUM_CASCADES 4
#define MAX_NUM_ATMOSPHERES 8

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Density is not pre-multiplied.
struct DepthFogParams
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  vec4 minMaxDensity; // Minimum density (x) and maximum density (y). z and w are unused.
};

// Density is pre-multiplied.
struct HeightFogParams
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  vec4 falloff; // Falloff (x). y, z and w are unused.
};

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

layout(rgba16f, binding = 0) restrict writeonly uniform image3D inScatExt;

layout(binding = 0) uniform sampler3D scatExtinction;
layout(binding = 1) uniform sampler3D emissionPhase;
layout(binding = 2) uniform sampler2D noise; // Blue noise
// TODO: Texture arrays.
layout(binding = 3) uniform sampler2D cascadeMaps;
layout(binding = 4) uniform sampler2DArray transLUTs;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

layout(std140, binding = 1) uniform VolumetricBlock
{
  DepthFogParams u_depthParams;
  HeightFogParams u_heightParams;
  vec4 u_lightDir; // Light direction (x, y, z). w is unused.
  vec4 u_lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  vec4 u_ambientColourIntensity; // Ambient colour (x, y, z) and intensity (w).
  ivec4 u_fogParams; // Number of OBB fog volumes (x), fog parameter bitmask (y). z and w are unused.
};

layout(std140, binding = 2) uniform CascadedShadowBlock
{
  mat4 u_lightVP[NUM_CASCADES];
  vec4 u_cascadeData[NUM_CASCADES]; // Cascade split distance (x). y, z and w are unused.
  vec4 u_shadowParams; // The constant bias (x), normal bias (y), the minimum PCF radius (z) and the cascade blend fraction (w).
};

// Temporal AA parameters. TODO: jittered camera matrices.
layout(std140, binding = 3) uniform TemporalBlock
{
  mat4 u_previousView;
  mat4 u_previousProj;
  mat4 u_previousVP;
  mat4 u_prevInvViewProjMatrix;
  vec4 u_prevPosTime;
};

layout(std140, binding = 4) uniform HillaireParams
{
  AtmosphereParams u_params[MAX_NUM_ATMOSPHERES];
};

layout(std430, binding = 5) readonly buffer HillaireIndices
{
  int u_atmosphereIndices[MAX_NUM_ATMOSPHERES];
};

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  vec4 temporal = fract((u_prevPosTime.wwww * vec4(4.0, 5.0, 6.0, 7.0)) * 0.61803399);
  vec2 uv = (vec2(coords) + 0.5.xx) / vec2(textureSize(noise, 0).xy);
  return fract(texture(noise, uv) + temporal);
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

      float minX = float(i) / float(NUM_CASCADES);
      float maxX = minX + 1.0 / float(NUM_CASCADES);
      projCoords.x = mix(minX, maxX, projCoords.x);

      shadowFactor = float(texture(cascadeMaps, projCoords.xy).r >= projCoords.z);
      break;
    }
  }

  return shadowFactor;
}

// Decode a Hillaire2020 LUT.
vec3 getValFromLUT(float atmIndex, sampler2DArray tex, vec3 pos, vec3 sunDir,
                   float groundRadiusMM, float atmosphereRadiusMM)
{
  float height = length(pos);
  vec3 up = pos / height;

  float sunCosZenithAngle = dot(sunDir, up);

  float u = clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0);
  float v = max(0.0, min(1.0, (height - groundRadiusMM) / (atmosphereRadiusMM - groundRadiusMM)));

  return texture(tex, vec3(u, v, atmIndex)).rgb;
}

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(inScatExt).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec3 dither = (2.0 * sampleDither(invoke.xy).rgb - 1.0.xxx) / vec3(numFroxels).xyz;
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) / vec2(numFroxels.xy) + dither.xy;

  vec4 temp = u_invViewProjMatrix * vec4(2.0 * uv - 1.0.xx, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  vec3 direction = worldSpaceMax - u_camPosition;
  float w = (float(invoke.z) + 0.5) / float(numFroxels.z) + dither.z;
  vec3 worldSpacePostion = u_camPosition + direction * w * w;

  vec3 uvw = vec3(uv, w);

  vec4 se = texture(scatExtinction, uvw);
  vec4 ep = texture(emissionPhase, uvw);

  // Light the voxel. Just cascaded shadow maps and voxel emission for now.
  float phaseFunction = getMiePhase(dot(normalize(direction), u_lightDir.xyz), ep.w);
  vec3 mieScattering = se.xyz;
  vec3 extinction = se.www;
  vec3 voxelAlbedo = max(mieScattering / extinction, 0.0.xxx);

  float visibility = (u_fogParams.y & (1 << 0)) != 0 ? 1.0 : cascadedShadow(worldSpacePostion);

  // Fetch the atmosphere.
  const int atmosphereIndex = u_atmosphereIndices[0];
  const AtmosphereParams params = u_params[atmosphereIndex];

  // Fetch the transmittance.
  vec3 sunTransmittance = getValFromLUT(float(atmosphereIndex), transLUTs,
                                        params.viewPos.xyz,
                                        normalize(u_lightDir.xyz),
                                        params.planetAlbedoRadius.w,
                                        params.sunDirAtmRadius.w);

  vec3 light = voxelAlbedo * phaseFunction * visibility * u_lightColourIntensity.xyz * u_lightColourIntensity.w;

  // Multiply by the transmittance from the sun -> object.
  light *= sunTransmittance;

  // Add ambient and emission contributions.
  light += ep.xyz;
  light += u_ambientColourIntensity.xyz * u_ambientColourIntensity.w * voxelAlbedo;

  imageStore(inScatExt, invoke, vec4(light, se.w));
}
