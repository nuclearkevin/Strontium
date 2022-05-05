#type compute
#version 460 core

#define PI 3.141592654
#define NUM_CASCADES 4

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) restrict writeonly uniform image3D inScatExt;

layout(binding = 0) uniform sampler3D scatExtinction;
layout(binding = 1) uniform sampler3D emissionPhase;
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
  vec4 u_lightDir; // Light direction (x, y, z). w is unused.
  vec4 u_lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  ivec4 u_numFogVolumes; // Number of OBB fog volumes (x). y, z and w are unused.
};

layout(std140, binding = 2) uniform CascadedShadowBlock
{
  mat4 u_lightVP[NUM_CASCADES];
  vec4 u_cascadeData[NUM_CASCADES]; // Cascade split distance (x). y, z and w are unused.
  vec4 u_shadowParams; // The constant bias (x), normal bias (y), the minimum PCF radius (z) and the cascade blend fraction (w).
};

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

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(inScatExt).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec2 uv = (vec2(invoke.xy) + 0.5.xx) / vec2(numFroxels.xy);

  vec4 temp = u_invViewProjMatrix * vec4(2.0 * uv - 1.0.xx, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  vec3 direction = worldSpaceMax - u_camPosition;
  float w = (float(invoke.z) + 0.5) / float(numFroxels.z);
  vec3 worldSpacePostion = u_camPosition + direction * w * w;

  vec4 se = texelFetch(scatExtinction, invoke, 0);
  vec4 ep = texelFetch(emissionPhase, invoke, 0);

  // Light the voxel. Just cascaded shadow maps and voxel emission for now.
  // TODO: Sngle sample along the shadowed light's direction to account for out scattering.
  float phaseFunction = getMiePhase(dot(normalize(direction), u_lightDir.xyz), ep.w);
  vec3 mieScattering = se.xyz;
  vec3 extinction = se.www;
  vec3 voxelAlbedo = mieScattering / extinction;

  float visibility = cascadedShadow(worldSpacePostion);

  vec3 light = max(voxelAlbedo * phaseFunction * visibility, 0.0.xxx) + ep.xyz;

  imageStore(inScatExt, invoke, vec4(light, se.w));
}
