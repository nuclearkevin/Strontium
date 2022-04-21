#type compute
#version 460 core
/*
 * A depth-aware bilateral blur used to smooth screen-space effects. Based off of:
 * https://github.com/nvpro-samples/gl_ssao/blob/master/hbao_blur.frag.glsl
*/

layout(local_size_x = 8, local_size_y = 8) in;

#define KERNEL_RADIUS 6
#define SHARPNESS 5.0

layout(rgba16f, binding = 0) restrict writeonly uniform image2D outImage;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D inTexture;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

layout(std140, binding = 1) uniform AOBlock
{
  vec4 u_aoParams1; // World-space radius (x), ao multiplier (y), ao exponent (z).
  vec4 u_aoParams2; // Blur direction (x, y). z and w are unused.
};

// https://stackoverflow.com/questions/51108596/linearize-depth
float linearizeDepth(float d, float zNear, float zFar)
{
  float zN = 2.0 * d - 1.0;
  return 2.0 * zNear * zFar / (zFar + zNear - zN * (zFar - zNear));
}

float blurFunction(vec2 uv, float r, float centerDepth, sampler2D depthMap,
                   sampler2D blurMap, vec2 nearFar, inout float totalWeight)
{
  float aoSample = texture(blurMap, uv).r;
  float d = linearizeDepth(textureLod(depthMap, uv, 1.0).r, nearFar.x, nearFar.y);

  const float blurSigma = float(KERNEL_RADIUS) * 0.5;
  const float blurFalloff = 1.0 / (2.0 * blurSigma * blurSigma);

  float ddiff = abs(d - centerDepth) * SHARPNESS;
  float w = exp2(-r * r * blurFalloff - ddiff * ddiff);
  totalWeight += w;

  return aoSample * w;
}

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  ivec2 outSize = ivec2(imageSize(outImage).xy);

  // Quit early for threads that aren't in bounds of the screen.
  if (any(greaterThanEqual(invoke, outSize)))
    return;

  vec2 texelSize = 1.0.xx / vec2(outSize);
  vec2 uvs = (vec2(invoke) + 0.5.xx) * texelSize;

  float center = texture(inTexture, uvs).r;
  float centerDepth = linearizeDepth(textureLod(gDepth, uvs, 1.0).r, u_nearFarGamma.x, u_nearFarGamma.y);

  float cTotal = center;
  float wTotal = 1.0;

  for (uint r = 1; r <= KERNEL_RADIUS; r++)
  {
    vec2 uv = uvs + (texelSize * float(r) * u_aoParams2.xy);
    cTotal += blurFunction(uv, r, centerDepth, gDepth, inTexture, u_nearFarGamma.xy, wTotal);
  }

  for (uint r = 1; r <= KERNEL_RADIUS; r++)
  {
    vec2 uv = uvs - (texelSize * float(r) * u_aoParams2.xy);
    cTotal += blurFunction(uv, r, centerDepth, gDepth, inTexture, u_nearFarGamma.xy, wTotal);
  }

  float result = cTotal / wTotal;

  imageStore(outImage, invoke, vec4(result.xxx, 1.0));
}
