#type compute
#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

struct OBBFogVolume
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  mat4 invTransformMatrix; // Inverse model-space transform matrix.
};

struct SphereFogVolume
{
  vec4 mieScatteringPhase; // Mie scattering (x, y, z) and phase value (w).
  vec4 emissionAbsorption; // Emission (x, y, z) and absorption (w).
  vec4 centerRadius; // Sphere center (x, y, z) and radius (w).
};

struct ScatteringParams
{
  vec4 mieScat; //  Mie scattering base (x, y, z) and density (w).
  vec4 mieAbs; //  Mie absorption base (x, y, z) and density (w).
  vec4 lightDirMiePhase; // Light direction (x, y, z) and the Mie phase (w).
  vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
};

layout(rgba16f, binding = 0) restrict writeonly uniform image3D scatExtinction;
layout(rgba16f, binding = 1) restrict writeonly uniform image3D emissionPhase;

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
  ivec4 u_numFogVolumes; // Number of OBB fog volumes (x). y, z and w are unused.
};

layout(std140, binding = 0) readonly buffer OBBFogVolumes
{
  OBBFogVolume u_volumes[];
};

bool pointInOBB(vec3 point, OBBFogVolume volume)
{
  vec3 tp = (volume.invTransformMatrix * vec4(point, 1.0)).xyz;
  bool xAxis = abs(tp.x) <= 1.0;
  bool yAxis = abs(tp.y) <= 1.0;
  bool zAxis = abs(tp.z) <= 1.0;
  return xAxis && yAxis && zAxis;
}

bool pointInSphere(vec3 point, SphereFogVolume volume)
{
  return length(volume.centerRadius.xyz - point) <= volume.centerRadius.w;
}

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(scatExtinction).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec2 uvs[4];
  uvs[0] = (vec2(invoke.xy) + vec2(0.0, 0.0)) / vec2(numFroxels.xy);
  uvs[1] = (vec2(invoke.xy) + vec2(1.0, 0.0)) / vec2(numFroxels.xy);
  uvs[2] = (vec2(invoke.xy) + vec2(0.0, 1.0)) / vec2(numFroxels.xy);
  uvs[3] = (vec2(invoke.xy) + vec2(1.0, 1.0)) / vec2(numFroxels.xy);

  vec3 worldSpacePostions[8];
  vec4 temp;
  vec3 worldSpaceMax;
  vec3 direction;
  float w;
  for (uint i = 0; i < 4; i++)
  {
    temp = u_invViewProjMatrix * vec4(2.0 * uvs[i] - 1.0.xx, 1.0, 1.0);
    worldSpaceMax = temp.xyz /= temp.w;
    direction = worldSpaceMax - u_camPosition;
    w = (float(invoke.z) + 0.0) / float(numFroxels.z);
    worldSpacePostions[i] = u_camPosition + normalize(direction) * length(direction) * w * w;
    w = (float(invoke.z) + 1.0) / float(numFroxels.z);
    worldSpacePostions[i + 4] = u_camPosition + normalize(direction) * length(direction) * w * w;
  }

  vec4 se = 0.0.xxxx;
  vec4 ep = 0.0.xxxx;

  float numVolumesInPixel = 0.0;
  for (uint i = 0; i < u_numFogVolumes.x; i++)
  {
    const OBBFogVolume volume = u_volumes[i];

    bool texelIntersects = false;
    for (uint i = 0; i < 8; i++)
    {
      bool test = pointInOBB(worldSpacePostions[i], volume);
      texelIntersects = texelIntersects || test;
    }

    if (texelIntersects)
    {
      float extinction = dot(volume.mieScatteringPhase.xyz, (1.0 / 3.0).xxx) + volume.emissionAbsorption.w;
      se += vec4(volume.mieScatteringPhase.xyz, extinction);
      ep += vec4(volume.emissionAbsorption.xyz, volume.mieScatteringPhase.w);

      numVolumesInPixel += 1.0;
    }
  }

  // Average phase.
  ep.w /= max(numVolumesInPixel, 1.0);

  imageStore(scatExtinction, invoke, se);
  imageStore(emissionPhase, invoke, ep);
}
