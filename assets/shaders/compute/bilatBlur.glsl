#type compute
#version 460 core
/*
 * A depth-aware bilateral blur used to smooth screen-space effects. Based off of:
 * https://github.com/nvpro-samples/gl_ssao/blob/master/hbao_blur.frag.glsl
*/

#define KERNEL_RADIUS 3
#define SHARPNESS 1.0

layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0) uniform sampler2D inTexture;
layout(binding = 3) uniform sampler2D gDepth;
layout(rgba16f, binding = 2) restrict writeonly uniform image2D outImage;

layout(location = 0) uniform vec2 u_direction; // Either (1, 0) for x or (0, 1) for y.

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y). z and w are unused.
};

// https://stackoverflow.com/questions/51108596/linearize-depth
float linearizeDepth(float d, float zNear, float zFar)
{
  float zN = 2.0 * d - 1.0;
  return 2.0 * zNear * zFar / (zFar + zNear - zN * (zFar - zNear));
}

vec4 blurFunction(vec2 uv, float r, float centerDepth, sampler2D depthMap,
                  sampler2D blurMap, vec2 nearFar, inout float totalWeight)
{
  vec4 volumetric = texture(blurMap, uv);
  float d = linearizeDepth(textureLod(depthMap, uv, 1.0).r, nearFar.x, nearFar.y);

  const float blurSigma = float(KERNEL_RADIUS) * 0.5;
  const float blurFalloff = 1.0 / (2.0 * blurSigma * blurSigma);

  float ddiff = (d - centerDepth) * SHARPNESS;
  float w = exp2(-r * r * blurFalloff - ddiff * ddiff);
  totalWeight += w;

  return volumetric * w;
}

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 texelSize = 1.0.xx / vec2(textureSize(inTexture, 0).xy);
  vec2 uvs = (vec2(invoke) + 0.5.xx) * texelSize;

  vec4 center = texture(inTexture, uvs);
  float centerDepth = linearizeDepth(textureLod(gDepth, uvs, 1.0).r, u_nearFar.x, u_nearFar.y);

  vec4 cTotal = center;
  float wTotal = 1.0;

  for (uint r = 1; r <= KERNEL_RADIUS; r++)
  {
    vec2 uv = uvs + (texelSize * float(r) * u_direction);
    cTotal += blurFunction(uv, r, centerDepth, gDepth, inTexture, u_nearFar.xy, wTotal);
  }

  for (uint r = 1; r <= KERNEL_RADIUS; r++)
  {
    vec2 uv = uvs - (texelSize * float(r) * u_direction);
    cTotal += blurFunction(uv, r, centerDepth, gDepth, inTexture, u_nearFar.xy, wTotal);
  }

  imageStore(outImage, invoke, cTotal / wTotal);
}
