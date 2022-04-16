#type compute
#version 460 core
/*
 * A depth-aware bilateral blur used to smooth screen-space effects. Based off of:
 * https://github.com/nvpro-samples/gl_ssao/blob/master/hbao_blur.frag.glsl
*/

layout(local_size_x = 8, local_size_y = 8) in;

#define KERNEL_RADIUS 6
#define SHARPNESS 5.0

// TODO: Get density from a volume texture instead.
struct ScatteringParams
{
  vec4 mieScat; //  Mie scattering base (x, y, z) and density (w).
  vec4 mieAbs; //  Mie absorption base (x, y, z) and density (w).
  vec4 lightDirMiePhase; // Light direction (x, y, z) and the Mie phase (w).
  vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
};

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

layout(std140, binding = 1) uniform GodrayBlock
{
  ScatteringParams u_params;
  vec4 u_godrayParams; // Blur direction (x, y), number of steps (z).
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
  vec4 aoSample = texture(blurMap, uv);
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

  vec4 center = texture(inTexture, uvs);
  float centerDepth = linearizeDepth(textureLod(gDepth, uvs, 1.0).r, u_nearFarGamma.x, u_nearFarGamma.y);

  vec4 cTotal = center;
  float wTotal = 1.0;

  for (uint r = 1; r <= KERNEL_RADIUS; r++)
  {
    vec2 uv = uvs + (2.0 * texelSize * float(r) * u_godrayParams.xy);
    cTotal += blurFunction(uv, float(r), centerDepth, gDepth, inTexture, u_nearFarGamma.xy, wTotal);
  }

  for (uint r = 1; r <= KERNEL_RADIUS; r++)
  {
    vec2 uv = uvs - (2.0 * texelSize * float(r) * u_godrayParams.xy);
    cTotal += blurFunction(uv, float(r), centerDepth, gDepth, inTexture, u_nearFarGamma.xy, wTotal);
  }

  imageStore(outImage, invoke, cTotal / max(wTotal, 1e-4));
}
