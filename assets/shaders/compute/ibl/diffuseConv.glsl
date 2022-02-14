#type compute
#version 460 core
/*
 * A compute shader to compute the diffuse lighting integral through
 * importance sampling the cosine lobe.
*/

#define TWO_PI 6.283185308
#define PI 3.141592654

layout(local_size_x = 8, local_size_y = 8) in;

// The environment map to convolute.
layout(binding = 0) uniform samplerCube environmentMap;

// The output irradiance map.
layout(rgba16f, binding = 1) restrict writeonly uniform imageCube irradianceMap;

// Function for converting between image coordiantes and world coordiantes.
vec3 cubeToWorld(ivec3 cubeCoord, vec2 cubeSize);

// Importance sample a cosine lobe.
vec3 importanceSampleCos(uint i, uint N, vec3 normal);

void main()
{
  vec2 cubemapSize = vec2(imageSize(irradianceMap).xy);
  vec3 worldPos = cubeToWorld(ivec3(gl_GlobalInvocationID), cubemapSize);

  // The normal is the same as the worldspace position.
  vec3 normal = normalize(worldPos);

  vec3 irradiance = vec3(0.0);
  const uint numSamples = 1024u;

  for (uint i = 0; i < numSamples; i++)
  {
    vec3 sampleDir = importanceSampleCos(i, numSamples, normal);
    irradiance += texture(environmentMap, sampleDir).rgb;
  }

  irradiance = PI * irradiance * (1.0 / float(numSamples));

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
