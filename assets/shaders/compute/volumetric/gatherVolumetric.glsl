#type compute
#version 460 core

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) restrict writeonly uniform image3D inScatTrans;

layout(binding = 0) uniform sampler3D inScatExt;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  ivec3 numFroxels = ivec3(imageSize(inScatTrans).xyz);

  if (any(greaterThanEqual(invoke, numFroxels.xy)))
    return;

  vec2 uvs = (vec2(invoke.xy) + 0.5.xx) / vec2(numFroxels.xy);

  vec4 temp = u_invViewProjMatrix * vec4(2.0 * uvs - 1.0.xx, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  vec3 direction = worldSpaceMax - u_camPosition;

  // Loop over the z dimension and integrate the in-scattered lighting and transmittance.
  float transmittance = 1.0;
  vec3 luminance = 0.0.xxx;
  float sampleTransmittance;
  vec3 scatteringIntegral;
  vec4 ie;

  vec3 worldSpacePostion;
  float dt;
  vec3 previousPosition = u_camPosition;
  float w;

  for (uint i = 0; i < numFroxels.z; ++i)
  {
    ie = texelFetch(inScatExt, ivec3(invoke, i), 0);

    if (ie.w < 1e-4)
    {
      luminance += ie.xyz;
      imageStore(inScatTrans, ivec3(invoke, i), vec4(luminance, transmittance));
      continue;
    }

    w = (float(i) + 0.5) / float(numFroxels.z);
    worldSpacePostion = u_camPosition + normalize(direction) * length(direction) * w * w;

    dt = length(worldSpacePostion - previousPosition);
    sampleTransmittance = exp(-dt * ie.w);
    scatteringIntegral = (ie.xyz - ie.xyz * sampleTransmittance) / ie.w;

    luminance += max(scatteringIntegral * transmittance, 0.0);
    transmittance *= min(sampleTransmittance, 1.0);

    previousPosition = worldSpacePostion;

    imageStore(inScatTrans, ivec3(invoke, i), vec4(luminance, transmittance));
  }
}
