#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of volumetric effects.
 */

#define PI 3.141592654

layout(local_size_x = 8, local_size_y = 8) in;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y). z and w are unused.
};

// Volumetric specific uniforms.
layout(std140, binding = 1) uniform VolumetricBlock
{
  ivec4 u_volumetricSettings; // Bitmask for volumetric settings (x). y, z and w are unused.
};

layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D godrays; // Screen-space godrays.

// Get the screen-space godray effect.
vec4 getGodrays(sampler2D godrayTex, sampler2D gDepth, vec2 gBufferCoords);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(textureSize(gDepth, 0).xy);

  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;

  if ((u_volumetricSettings.x & (1 << 0)) != 0)
  {
    vec4 godrays = getGodrays(godrays, gDepth, gBufferUVs);
    totalRadiance *= godrays.a;
    totalRadiance += godrays.rgb;
  }

  imageStore(lightingBuffer, invoke, vec4(totalRadiance, 1.0));
}

//------------------------------------------------------------------------------
// Joined bilateral upsample from:
// - https://www.shadertoy.com/view/3dK3zR
// - https://bartwronski.com/2019/09/22/local-linear-models-guided-filter/
// - https://johanneskopf.de/publications/jbu/paper/FinalPaper_0185.pdf
//------------------------------------------------------------------------------
float gaussian(float sigma, float x)
{
  return exp(-(x * x) / (2.0 * sigma * sigma));
}

vec4 getGodrays(sampler2D godrayTex, sampler2D gDepth, vec2 gBufferCoords)
{
  vec2 halfTexSize = vec2(textureSize(gDepth, 1).xy);
  vec2 halfTexelSize = 1.0.xx / halfTexSize;
  vec2 pixel = gBufferCoords * halfTexSize + 0.5.xx;
  vec2 f = fract(pixel);
  pixel = (floor(pixel) / halfTexSize) - (halfTexelSize / 2.0);

  float fullResDepth = textureLod(gDepth, gBufferCoords, 0.0).r;

  float halfResDepth[4];
  halfResDepth[0] = textureLodOffset(gDepth, pixel, 1.0, ivec2(0, 0)).r;
  halfResDepth[1] = textureLodOffset(gDepth, pixel, 1.0, ivec2(0, 1)).r;
  halfResDepth[2] = textureLodOffset(gDepth, pixel, 1.0, ivec2(1, 0)).r;
  halfResDepth[3] = textureLodOffset(gDepth, pixel, 1.0, ivec2(1, 1)).r;

  float sigmaV = 0.002;
  float weights[4];
  weights[0] = gaussian(sigmaV, abs(fullResDepth - halfResDepth[0])) * (1.0 - f.x) * (1.0 - f.y);
  weights[1] = gaussian(sigmaV, abs(fullResDepth - halfResDepth[1])) * (1.0 - f.x) * f.y;
  weights[2] = gaussian(sigmaV, abs(fullResDepth - halfResDepth[2])) * f.x * (1.0 - f.y);
  weights[3] = gaussian(sigmaV, abs(fullResDepth - halfResDepth[3])) * f.x * f.y;

  vec4 halfResEffect[4];
  halfResEffect[0] = textureOffset(godrayTex, pixel, ivec2(0, 0));
  halfResEffect[1] = textureOffset(godrayTex, pixel, ivec2(0, 1));
  halfResEffect[2] = textureOffset(godrayTex, pixel, ivec2(1, 0));
  halfResEffect[3] = textureOffset(godrayTex, pixel, ivec2(1, 1));

  vec4 result = halfResEffect[0] * weights[0] + halfResEffect[1] * weights[1]
              + halfResEffect[2] * weights[2] + halfResEffect[3] * weights[3];

  float weightSum = max(weights[0] + weights[1] + weights[2] + weights[3], 1e-4);
  return result / weightSum;
}
