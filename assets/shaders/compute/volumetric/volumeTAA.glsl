#type compute
#version 460 core

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

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(resolveInScatExt).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec3 texel = 1.0.xxx / vec3(numFroxels.xyz);
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) * texel.xy;
  float w = (float(invoke.z) + 0.5.x) * texel.z;

  //----------------------------------------------------------------------------
  // Quick and dirty box blur for spatial denoising.
  vec4 current = 0.0.xxxx;
  current += texture(currentInScatExt, vec3(uv, w) + vec3(-0.5, -0.5, -0.5) * texel);
  current += texture(currentInScatExt, vec3(uv, w) + vec3(0.5, -0.5, -0.5) * texel);
  current += texture(currentInScatExt, vec3(uv, w) + vec3(-0.5, 0.5, -0.5) * texel);
  current += texture(currentInScatExt, vec3(uv, w) + vec3(0.5, 0.5, -0.5) * texel);
  current += texture(currentInScatExt, vec3(uv, w) + vec3(-0.5, -0.5, 0.5) * texel);
  current += texture(currentInScatExt, vec3(uv, w) + vec3(0.5, -0.5, 0.5) * texel);
  current += texture(currentInScatExt, vec3(uv, w) + vec3(-0.5, 0.5, 0.5) * texel);
  current += texture(currentInScatExt, vec3(uv, w) + vec3(0.5, 0.5, 0.5) * texel);
  current /= 8.0;
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
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

  vec4 result;
  if (inFrustum(previousPosNDC))
    result = mix(history, current, 0.05);
  else
    result = current;
  //----------------------------------------------------------------------------

  // Store the (mostly) denoised result.
  imageStore(resolveInScatExt, invoke, result);
}
