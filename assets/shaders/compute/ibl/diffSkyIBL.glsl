#type compute
#version 460 core
/*
 * A compute shader to compute the diffuse lighting integral through
 * importance sampling the cosine lobe. Modified to sample the sky view LUT of
 * Hillaire2020.
*/

#define TWO_PI 6.283185308
#define PI 3.141592654
#define PI_OVER_TWO 1.570796327

layout(local_size_x = 8, local_size_y = 8) in;

// The skyview LUT to convolve.
layout(binding = 0) uniform sampler2D skyviewLUT;

// The output irradiance map.
layout(rgba16f, binding = 1) restrict writeonly uniform imageCube irradianceMap;

// Skybox specific uniforms
layout(std140, binding = 2) uniform SkyboxBlock
{
  vec4 u_lodDirection; // IBL lod (x), sun direction (y, z, w).
  vec4 u_sunIntensitySizeGRadiusARadius; // Sun intensity (x), size (y), ground radius (z) and atmosphere radius (w).
  vec4 u_viewPosSkyIntensity; // View position (x, y, z) and sky intensity (w).
  ivec4 u_skyboxParams2; // The skybox to use (x). y, z and w are unused.
};

// Function for converting between image coordiantes and world coordiantes.
vec3 cubeToWorld(ivec3 cubeCoord, vec2 cubeSize);
// Sample the sky view LUT.
vec3 sampleSkyViewLUT(sampler2D lut, vec3 viewPos, vec3 viewDir,
                      vec3 sunDir, float groundRadiusMM);

// Importance sample a cosine lobe.
vec3 importanceSampleCos(uint i, uint N, vec3 normal);

void main()
{
  vec2 cubemapSize = vec2(imageSize(irradianceMap).xy);
  vec3 worldPos = cubeToWorld(ivec3(gl_GlobalInvocationID), cubemapSize);

  vec3 sunPos = normalize(u_lodDirection.yzw);
  vec3 viewPos = u_viewPosSkyIntensity.xyz;
  float groundRadiusMM =  u_sunIntensitySizeGRadiusARadius.z;

  // The normal is the same as the worldspace position.
  vec3 normal = normalize(worldPos);
  normal.xz *= -1.0;

  vec3 irradiance = vec3(0.0);
  const uint numSamples = 512;

  for (uint i = 0; i < numSamples; i++)
  {
    vec3 sampleDir = importanceSampleCos(i, numSamples, normal);
    irradiance += sampleSkyViewLUT(skyviewLUT, viewPos, sampleDir, sunPos, groundRadiusMM);
  }

  irradiance = PI * irradiance * (1.0 / float(numSamples)) * u_viewPosSkyIntensity.w;
  imageStore(irradianceMap, ivec3(gl_GlobalInvocationID), vec4(irradiance, 1.0));
}

// I need to figure out how to make these branchless one of these days...
vec3 cubeToWorld(ivec3 cubeCoord, vec2 cubeSize)
{
  vec2 texCoord = vec2(cubeCoord.xy) / cubeSize;
  texCoord = texCoord  * 2.0 - 1.0; // Swap to -1 -> +1
  switch(cubeCoord.z)
  {
    case 0: return vec3(1.0, -texCoord.yx); // CUBE_MAP_POS_X
    case 1: return vec3(-1.0, -texCoord.y, texCoord.x); // CUBE_MAP_NEG_X
    case 2: return vec3(texCoord.x, 1.0, texCoord.y); // CUBE_MAP_POS_Y
    case 3: return vec3(texCoord.x, -1.0, -texCoord.y); // CUBE_MAP_NEG_Y
    case 4: return vec3(texCoord.x, -texCoord.y, 1.0); // CUBE_MAP_POS_Z
    case 5: return vec3(-texCoord.xy, -1.0); // CUBE_MAP_NEG_Z
  }
  return vec3(0.0);
}

float safeACos(float x)
{
  return acos(clamp(x, -1.0, 1.0));
}

vec3 sampleSkyViewLUT(sampler2D lut, vec3 viewPos, vec3 viewDir,
                      vec3 sunDir, float groundRadiusMM)
{
  float height = length(viewPos);
  vec3 up = viewPos / height;

  float horizonAngle = safeACos(sqrt(height * height - groundRadiusMM * groundRadiusMM) / height);
  float altitudeAngle = horizonAngle - acos(dot(viewDir, up));

  vec3 right = cross(sunDir, up);
  vec3 forward = cross(up, right);

  vec3 projectedDir = normalize(viewDir - up * (dot(viewDir, up)));
  float sinTheta = dot(projectedDir, right);
  float cosTheta = dot(projectedDir, forward);
  float azimuthAngle = atan(sinTheta, cosTheta) + PI;

  float u = azimuthAngle / (TWO_PI);
  float v = 0.5 + 0.5 * sign(altitudeAngle) * sqrt(abs(altitudeAngle) / PI_OVER_TWO);

  return texture(lut, vec2(u, v)).rgb;
}

// Hammersley sequence, generates a low discrepancy pseudorandom number.
vec2 Hammersley(uint i, uint N)
{
  float fbits;
  uint bits = i;

  bits  = (bits << 16u) | (bits >> 16u);
  bits  = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits  = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits  = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits  = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  fbits = float(bits) * 2.3283064365386963e-10;

  return vec2(float(i) / float(N), fbits);
}

// Importance sample a cosine lobe.
vec3 importanceSampleCos(uint i, uint N, vec3 normal)
{
  vec2 xi = Hammersley(i, N);

  float phi = TWO_PI * xi.x;
  float theta = safeACos(sqrt(xi.y));

  float sinTheta = sin(theta);
  vec3 world = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cos(theta));

  vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, normal));
	vec3 bitangent = cross(normal, tangent);
  return normalize(tangent * world.x + bitangent * world.y + normal * world.z);
}
