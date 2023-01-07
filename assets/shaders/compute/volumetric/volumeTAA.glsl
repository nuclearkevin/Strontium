#type compute
#version 460 core

#define EXP_AVERAGE_MIX 0.05
//#define USE_NEIGHBOURHOOD_CLAMP

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) restrict writeonly uniform image3D resolveInScatExt;

layout(binding = 0) uniform sampler3D currentInScatExt;
layout(binding = 1) uniform sampler3D historyInScatExt;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
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

bool inFrustum(vec3 ndc)
{
  bool xAxis = ndc.x >= -1.0 && ndc.x <= 1.0;
  bool yAxis = ndc.y >= -1.0 && ndc.y <= 1.0;
  bool zAxis = ndc.z >= -1.0 && ndc.z <= 1.0;

  return xAxis && yAxis && zAxis;
}

void neighbourhoodMinMaxCross(vec3 uvw, vec3 texel, out vec4 minCross, out vec4 maxCross)
{
  minCross = texture(currentInScatExt, uvw);
  maxCross = minCross;
  vec4 adjacents[6];
  adjacents[0] = texture(currentInScatExt, uvw + vec3(0.0, -1.0, 0.0) * texel);
  adjacents[1] = texture(currentInScatExt, uvw + vec3(0.0, 1.0, 0.0) * texel);
  adjacents[2] = texture(currentInScatExt, uvw + vec3(-1.0, 0.0, 0.0) * texel);
  adjacents[3] = texture(currentInScatExt, uvw + vec3(1.0, 0.0, 0.0) * texel);
  adjacents[4] = texture(currentInScatExt, uvw + vec3(0.0, 0.0, -1.0) * texel);
  adjacents[5] = texture(currentInScatExt, uvw + vec3(0.0, 0.0, 1.0) * texel);

  for (uint i = 0; i < 6; i++)
  {
    minCross = min(adjacents[i], minCross);
    maxCross = max(adjacents[i], maxCross);
  }
}

void neighbourhoodMinMax(vec3 uvw, vec3 texel, out vec4 nMin, out vec4 nMax)
{
  vec4 minCross, maxCross;
  neighbourhoodMinMaxCross(uvw, texel, minCross, maxCross);

  vec4 corners[8];
  corners[0] = texture(currentInScatExt, uvw + vec3(-1.0, -1.0, -1.0) * texel);
  corners[1] = texture(currentInScatExt, uvw + vec3(1.0, -1.0, -1.0) * texel);
  corners[2] = texture(currentInScatExt, uvw + vec3(-1.0, 1.0, -1.0) * texel);
  corners[3] = texture(currentInScatExt, uvw + vec3(1.0, 1.0, -1.0) * texel);
  corners[4] = texture(currentInScatExt, uvw + vec3(-1.0, -1.0, 1.0) * texel);
  corners[5] = texture(currentInScatExt, uvw + vec3(1.0, -1.0, 1.0) * texel);
  corners[6] = texture(currentInScatExt, uvw + vec3(-1.0, 1.0, 1.0) * texel);
  corners[7] = texture(currentInScatExt, uvw + vec3(1.0, 1.0, 1.0) * texel);

  for (uint i = 0; i < 8; i++)
  {
    nMin = min(corners[i], nMin);
    nMax = max(corners[i], nMax);
  }

  nMin = 0.5 * nMin + 0.5 * minCross;
  nMax = 0.5 * nMax + 0.5 * maxCross;
}

vec4 neighbourhoodClamp(vec4 history, vec3 uvw, vec3 texel)
{
  vec4 cMin, cMax;
  neighbourhoodMinMax(uvw, texel, cMin, cMax);

  return clamp(history, cMin, cMax);
}

vec4 sampleGaussian(sampler3D tex, vec3 coord, vec3 offset, inout float totalWeight)
{
  float weight = exp(-2.29 * dot(coord - offset, coord - offset));
  totalWeight += weight;
  return weight * texture(tex, coord + offset);
}

vec4 spatialFilter(sampler3D tex, vec3 coord)
{
  vec3 texel = 1.0.xxx / vec3(textureSize(tex, 0).xyz);

  vec4 accum = 0.0.xxxx;
  float totalWeight = 0.0;

  accum += sampleGaussian(tex, coord, texel * vec3(-1, -1, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(-1, -1, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(-1, -1, 1), totalWeight);

  accum += sampleGaussian(tex, coord, texel * vec3(-1, 0, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(-1, 0, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(-1, 0, 1), totalWeight);

  accum += sampleGaussian(tex, coord, texel * vec3(-1, 1, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(-1, 1, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(-1, 1, 1), totalWeight);

  accum += sampleGaussian(tex, coord, texel * vec3(0, -1, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(0, -1, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(0, -1, 1), totalWeight);

  accum += sampleGaussian(tex, coord, texel * vec3(0, 0, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(0, 0, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(0, 0, 1), totalWeight);

  accum += sampleGaussian(tex, coord, texel * vec3(0, 1, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(0, 1, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(0, 1, 1), totalWeight);

  accum += sampleGaussian(tex, coord, texel * vec3(1, -1, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(1, -1, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(1, -1, 1), totalWeight);

  accum += sampleGaussian(tex, coord, texel * vec3(1, 0, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(1, 0, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(1, 0, 1), totalWeight);

  accum += sampleGaussian(tex, coord, texel * vec3(1, 1, -1), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(1, 1, 0), totalWeight);
  accum += sampleGaussian(tex, coord, texel * vec3(1, 1, 1), totalWeight);

  return max(accum / totalWeight, 0.0.xxxx);
}

void main()
{
  const uint lInvoke = uint(gl_LocalInvocationIndex);
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(resolveInScatExt).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec3 texel = 1.0.xxx / vec3(numFroxels.xyz);
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) * texel.xy;
  float w = (float(invoke.z) + 0.5.x) * texel.z;

  // Spatially filter the current sample.
  vec4 average = spatialFilter(currentInScatExt, vec3(uv, w));

  // Temporal AA.
  // Get the current world space position.
  vec4 temp = u_invViewProjMatrix * vec4(2.0 * uv - 1.0.xx, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  vec3 direction = worldSpaceMax - u_camPosition;
  vec3 worldSpacePostion = u_camPosition + direction * w * w;

  // Project the current world space position to the previous frame.
  temp = u_previousVP * vec4(worldSpacePostion, 1.0);
  vec3 previousPosNDC = temp.xyz / temp.w;
  vec2 previousUV = 0.5 * previousPosNDC.xy + 0.5.xx;

  // Transform the z component for sampling.
  temp = u_prevInvViewProjMatrix * vec4(2.0 * previousUV - 1.0.xx, 1.0, 1.0);
  vec3 prevWorldSpaceMax = temp.xyz / temp.w;
  float prevMaxT = length(prevWorldSpaceMax - u_prevPosTime.xyz);
  float prevZ = length(worldSpacePostion - u_prevPosTime.xyz);
  prevZ /= prevMaxT;
  prevZ = pow(prevZ, 1.0 / 2.0);

  vec4 history = texture(historyInScatExt, vec3(previousUV, prevZ));

  #ifdef USE_NEIGHBOURHOOD_CLAMP
  history = neighbourhoodClamp(history, vec3(uv, w), texel);
  #endif

  vec4 result;
  if (inFrustum(previousPosNDC))
    result = mix(history, average, EXP_AVERAGE_MIX);
  else
    result = average;
  //----------------------------------------------------------------------------

  // Store the (mostly) denoised result.
  imageStore(resolveInScatExt, invoke, result);
}
