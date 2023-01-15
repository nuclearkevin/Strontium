#type compute
#version 460 core

#define PI 3.141592654
#define NUM_CASCADES 4

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

layout(rgba16f, binding = 0) restrict uniform image3D inScatExt;

layout(binding = 0) uniform sampler3D scatExtinction;
layout(binding = 1) uniform sampler3D emissionPhase;
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
  vec4 u_frustumCenter; // Frustum center (x, y, z). w is unused.
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

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  vec4 temporal = fract((u_prevPosTime.wwww * vec4(4.0, 5.0, 6.0, 7.0)) * 0.61803399);
  vec2 uv = (vec2(coords) + 0.5.xx) / vec2(textureSize(noise, 0).xy);
  return fract(texture(noise, uv) + temporal);
}

float getMiePhase(float cosTheta, float g)
{
  const float scale = 1.0 / (4.0 * PI);

  float num = (1.0 - g * g);
  float denom = pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
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
  vec3 totalInScatExt = imageLoad(inScatExt, invoke).rgb;

  // Light the voxel. Just cascaded shadow maps and voxel emission for now.
  // TODO: Single sample along the shadowed light's direction to account for out scattering.
  float phaseFunction = getMiePhase(dot(normalize(direction), u_lightDir.xyz), ep.w);
  vec3 mieScattering = se.xyz;
  vec3 extinction = se.www;
  vec3 voxelAlbedo = max(mieScattering / extinction, 0.0.xxx);

  vec3 light = voxelAlbedo * phaseFunction * u_lightColourIntensity.xyz * u_lightColourIntensity.w;
  totalInScatExt += light;

  imageStore(inScatExt, invoke, vec4(totalInScatExt, se.w));
}
