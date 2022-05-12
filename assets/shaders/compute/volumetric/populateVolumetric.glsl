#type compute
#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Density is pre-multiplied.
struct OBBFogVolume
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  mat4 invTransformMatrix; // Inverse model-space transform matrix.
};

// Density is not pre-multiplied.
struct DepthFogParams
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  vec4 minMaxDensity; // Minimum density (x) and maximum density (y).
};

// Density is pre-multiplied.
struct HeightFogParams
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  vec4 falloff;
};

layout(rgba16f, binding = 0) restrict writeonly uniform image3D scatExtinction;
layout(rgba16f, binding = 1) restrict writeonly uniform image3D emissionPhase;

layout(binding = 2) uniform sampler2D noise; // Blue noise

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

// Temporal AA parameters. TODO: jittered camera matrices.
layout(std140, binding = 3) uniform TemporalBlock
{
  mat4 u_previousView;
  mat4 u_previousProj;
  mat4 u_previousVP;
  mat4 u_prevInvViewProjMatrix;
  vec4 u_prevPosTime;
};

layout(std140, binding = 0) readonly buffer OBBFogVolumes
{
  OBBFogVolume u_volumes[];
};

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  vec4 temporal = fract((u_prevPosTime.wwww + vec4(0.0, 1.0, 2.0, 3.0)) * 0.61803399);
  vec2 uv = (vec2(coords) + 0.5.xx) / vec2(textureSize(noise, 0).xy);
  return fract(texture(noise, uv) + temporal);
}

bool pointInOBB(vec3 point, OBBFogVolume volume)
{
  vec3 tp = (volume.invTransformMatrix * vec4(point, 1.0)).xyz;
  bool xAxis = abs(tp.x) <= 1.0;
  bool yAxis = abs(tp.y) <= 1.0;
  bool zAxis = abs(tp.z) <= 1.0;
  return xAxis && yAxis && zAxis;
}

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(scatExtinction).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec3 dither = (2.0 * sampleDither(invoke.xy).rgb - 1.0.xxx) / vec3(numFroxels).xyz;

  // Compute the voxel center position.
  vec2 centerUVs;
  vec3 centerWorldPos;
  vec4 temp;
  vec3 worldSpaceMax;
  vec3 direction;
  float w;
  centerUVs = (vec2(invoke.xy) + 0.5.xx) / vec2(numFroxels.xy) + dither.xy;
  w = (float(invoke.z) + 0.5) / float(numFroxels.z) + dither.z;
  temp = u_invViewProjMatrix * vec4(2.0 * centerUVs - 1.0.xx, 1.0, 1.0);
  worldSpaceMax = temp.xyz /= temp.w;
  direction = worldSpaceMax - u_camPosition;
  centerWorldPos = u_camPosition + direction * w * w;

  vec4 se = 0.0.xxxx;
  vec4 ep = 0.0.xxxx;

  float numVolumesInPixel = 0.0;
  OBBFogVolume volume;
  float extinction;
  for (uint i = 0; i < u_fogParams.x; i++)
  {
    volume = u_volumes[i];

    if (pointInOBB(centerWorldPos, volume))
    {
      extinction = dot(volume.mieScatteringPhase.xyz, (1.0 / 3.0).xxx) + volume.emissionAbsorption.w;
      se += vec4(volume.mieScatteringPhase.xyz, extinction);
      ep += vec4(volume.emissionAbsorption.xyz, volume.mieScatteringPhase.w);

      numVolumesInPixel += 1.0;
    }
  }

  // Global depth fog.
  float density;
  if ((u_fogParams.y & (1 << 0)) != 0)
  {
    density = mix(u_depthParams.minMaxDensity.x, u_depthParams.minMaxDensity.y, w * w);

    extinction = dot(u_depthParams.mieScatteringPhase.xyz, (1.0 / 3.0).xxx) + u_depthParams.emissionAbsorption.w;
    se += vec4(u_depthParams.mieScatteringPhase.xyz, extinction) * density;
    ep += vec4(u_depthParams.emissionAbsorption.xyz * density, u_depthParams.mieScatteringPhase.w);

    numVolumesInPixel += 1.0;
  }

  // Global height fog.
  if ((u_fogParams.y & (1 << 1)) != 0)
  {
    density = exp(-max(centerWorldPos.y, 0.0) * u_heightParams.falloff.x);

    extinction = dot(u_heightParams.mieScatteringPhase.xyz, (1.0 / 3.0).xxx) + u_heightParams.emissionAbsorption.w;
    se += vec4(u_heightParams.mieScatteringPhase.xyz, extinction) * density;
    ep += vec4(u_heightParams.emissionAbsorption.xyz * density, u_heightParams.mieScatteringPhase.w);

    numVolumesInPixel += 1.0;
  }

  // Average phase.
  ep.w /= max(numVolumesInPixel, 1.0);

  imageStore(scatExtinction, invoke, se);
  imageStore(emissionPhase, invoke, ep);
}
