#type compute
#version 460 core
/*
 * A compute shader to compute screenspace horizon-based AO. Based off of the sample
 * implementation provided by NVIDIA:
 * https://github.com/nvpro-samples/gl_ssao/blob/master/hbao.frag.glsl
 * Effect is computed at quarter resolution.
*/

#define PI 3.141592654

#define NUM_STEPS 4
#define NUM_DIRECTIONS 8
#define BIAS 1e-6

layout(local_size_x = 8, local_size_y = 8) in;

// The output texture for the AO.
layout(rgba16f, binding = 0) restrict writeonly uniform image2D aoOut;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D noise;

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
  vec4 u_aoParams2; // Blur direction (x, y). Z and w are unused.
};

float computeHBAO(ivec2 coords, vec4 aoParams, sampler2D depthTex,
                  mat4 view, mat4 invVP);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);

  // Quit early for threads that aren't in bounds of the screen.
  if (any(greaterThanEqual(invoke, imageSize(aoOut).xy)))
    return;

  float ao = computeHBAO(invoke, u_aoParams1, gDepth, u_viewMatrix,
                         u_invViewProjMatrix);

  float result = pow(ao, u_aoParams1.z);
  imageStore(aoOut, invoke, vec4(result.xxx, 1.0));
}

// Fade out effects near the edge of the screen.
float screenFade(vec2 uv)
{
  vec2 fade = max(0.0f.xx, 12.0f * abs(uv - 0.5f.xx) - 5.0f);
  return clamp(1.0f - dot(fade, fade), 0.0, 1.0);
}

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  const vec2 noiseScale = vec2(textureSize(gDepth, 0).xy) / vec2(textureSize(noise, 0).xy);
  return texture(noise, (vec2(coords) + 0.5.xx) * noiseScale);
}

// Interleaved gradient noise from:
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
// Create a rotation matrix with IGL.
mat2 randomRotation()
{
  const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
  float theta = 2.0 * PI * fract(magic.z * fract(dot(vec2(gl_GlobalInvocationID.xy), magic.xy)));
  float sinTheta = sin(theta);
  float cosTheta = cos(theta);
  return mat2(cosTheta, sinTheta, -sinTheta, cosTheta);
}

// Create a normal rotation matrix given an angle.
mat2 rotation(float theta)
{
  float sinTheta = sin(theta);
  float cosTheta = cos(theta);
  return mat2(cosTheta, sinTheta, -sinTheta, cosTheta);
}

// HBAO falloff.
float falloff(float distanceSquare, float invR2)
{
  return distanceSquare * -invR2 + 1.0;
}

// Decodes the worldspace position of the fragment from depth.
vec3 decodePosition(ivec2 coords, sampler2D depthMap, mat4 invVP)
{
  float depth = texelFetch(depthMap, coords, 1).r;
  vec2 texCoords = (vec2(coords) + 0.5.xx) / vec2(textureSize(depthMap, 1).xy);
  vec3 clipCoords = 2.0 * vec3(texCoords, depth) - 1.0.xxx;
  vec4 temp = invVP * vec4(clipCoords, 1.0);
  return temp.xyz / temp.w;
}

vec4 decodePositionWithDepth(ivec2 coords, sampler2D depthMap, mat4 invVP)
{
  float depth = texelFetch(depthMap, coords, 1).r;
  vec2 texCoords = (vec2(coords) + 0.5.xx) / vec2(textureSize(depthMap, 1).xy);
  vec3 clipCoords = 2.0 * vec3(texCoords, depth) - 1.0.xxx;
  vec4 temp = invVP * vec4(clipCoords, 1.0);
  return vec4(temp.xyz / temp.w, depth);
}

// Reconstruct the view-space normal from depth
// (normal maps don't play well with SSAO).
// TODO: Cache the lookups to minimize matrix multiplications.
vec3 reconstructNormal(ivec2 centerCoord, sampler2D depthMap, vec3 center,
                       float centerDepth, mat4 invVP)
{
  const ivec2 leftUV = centerCoord - ivec2(1.0, 0.0);
  const ivec2 rightUV = centerCoord + ivec2(1.0, 0.0);
  const ivec2 downUV = centerCoord - ivec2(0.0, 1.0);
  const ivec2 upUV = centerCoord + ivec2(0.0, 1.0);

  const vec4 left = decodePositionWithDepth(leftUV, depthMap, invVP);
  const vec4 right = decodePositionWithDepth(rightUV, depthMap, invVP);
  const vec4 down = decodePositionWithDepth(downUV, depthMap, invVP);
  const vec4 up = decodePositionWithDepth(upUV, depthMap, invVP);

  // 0 is left, 1 is right, 2 is bottom, 3 is top.
  uint bestHor = abs(left.w - centerDepth) < abs(right.w - centerDepth) ? 0 : 1;
  uint bestVer = abs(down.w - centerDepth) < abs(up.w - centerDepth) ? 2 : 3;

  vec3 normal;
  if (bestHor == 0 && bestVer == 2)
  {
    normal = cross(left.xyz - center, down.xyz - center);
  }
  else if (bestHor == 0 && bestVer == 3)
  {
    normal = cross(up.xyz - center, left.xyz - center);
  }
  else if (bestHor == 1 && bestVer == 2)
  {
    normal = cross(down.xyz - center, right.xyz - center);
  }
  else if (bestHor == 1 && bestVer == 3)
  {
    normal = cross(right.xyz - center, up.xyz - center);
  }

  return normalize(normal);
}

//------------------------------------------------------------------------------
// P = view-space position at the kernel center.
// N = view-space normal at the kernel center.
// S = view-space position of the current sample.
//------------------------------------------------------------------------------
float computeAO(vec3 p, vec3 n, vec3 s, float invR2)
{
  vec3 v = s - p;
  float vDotV = max(dot(v, v), 1e-4);
  float nDotV = dot(n, v) * 1.0 / sqrt(vDotV);

  return clamp(nDotV - BIAS, 0.0, 1.0) * clamp(falloff(vDotV, invR2), 0.0, 1.0);
}

float computeHBAO(ivec2 coords, vec4 aoParams, sampler2D depthTex,
                  mat4 view, mat4 invVP)
{
  // Compute the view-space position and normal.
  vec4 pos = decodePositionWithDepth(coords, depthTex, invVP);
  vec3 viewPos = (view * vec4(pos.xyz, 1.0)).xyz;
  vec3 normal = reconstructNormal(coords, depthTex, pos.xyz, pos.w, invVP);
  vec3 viewNormal = normalize((view * vec4(normal, 0.0)).xyz);

  vec2 halfResTexel = vec2(textureSize(depthTex, 1).xy);
  vec2 halfResTexCoords = (vec2(coords) + 0.5.xx) / halfResTexel;

  // Screen-space radius of the sample disk.
  float sRadius = aoParams.x / max(abs(viewPos.z), 1e-4);
  sRadius = max(sRadius, float(NUM_STEPS));
  const float invR2 = 1.0 / (aoParams.x * aoParams.x);

  const float stepSize = sRadius / (float(NUM_STEPS) + 1.0);
  const float alpha = 2.0 * PI / float(NUM_DIRECTIONS);

  // Gather AO within the sample disk.
  float ao = 0.0;

  // Compute a jittered starting position.
  mat2 outerRot = randomRotation();
  float ray = sampleDither(coords).r * stepSize;
  // Outer loop rotates about the sample point.
  for (int i = 0; i < NUM_DIRECTIONS; i++)
  {
    // Compute a jittered direction vector.
    float theta = alpha * float(i);
    vec2 direction = rotation(theta) * outerRot * vec2(1.0, 0.0);

    // Inner loop raymarches through the depth buffer gathering AO contributions.
    for (uint j = 0; j < NUM_STEPS; j++)
    {
      vec2 snappedUV = round(direction * ray) / halfResTexel + halfResTexCoords;
      vec3 samplePos = (view * vec4(decodePosition(ivec2(snappedUV * halfResTexel), depthTex, invVP), 1.0)).xyz;

      float fade = screenFade(snappedUV);
      ao += computeAO(viewPos, viewNormal, samplePos, invR2) * fade;

      ray += stepSize;
    }
  }

  ao *= aoParams.y / (float(NUM_STEPS) * float(NUM_DIRECTIONS));

  return clamp(1.0 - 2.0 * ao, 0.0, 1.0);
}
