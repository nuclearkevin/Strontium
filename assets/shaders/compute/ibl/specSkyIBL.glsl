#type compute
#version 460 core
/*
 * A compute shader to compute the prefilter component of the split-sum
 * approach for environmental specular lighting. Modified to sample the sky
 * view LUT of Hillaire2020.
 * Many thanks to http://alinloghin.com/articles/compute_ibl.html, some of the
 * code here was adapted from their excellent article.
*/

#define MAX_NUM_ATMOSPHERES 8
#define MAX_NUM_DYN_SKY_IBL 8
#define TWO_PI 6.283185308
#define PI 3.141592654
#define PI_OVER_TWO 1.570796327

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

//------------------------------------------------------------------------------
// Params from the sky atmosphere pass to read the sky-view LUT.
// Atmospheric scattering coefficients are expressed per unit km.
struct ScatteringParams
{
  vec4 rayleighScat; //  Rayleigh scattering base (x, y, z) and height falloff (w).
  vec4 rayleighAbs; //  Rayleigh absorption base (x, y, z) and height falloff (w).
  vec4 mieScat; //  Mie scattering base (x, y, z) and height falloff (w).
  vec4 mieAbs; //  Mie absorption base (x, y, z) and height falloff (w).
  vec4 ozoneAbs; //  Ozone absorption base (x, y, z) and scale (w).
};

// Radii are expressed in megameters (MM).
struct AtmosphereParams
{
  ScatteringParams sParams;
  vec4 planetAlbedoRadius; // Planet albedo (x, y, z) and radius.
  vec4 sunDirAtmRadius; // Sun direction (x, y, z) and atmosphere radius (w).
  vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  vec4 viewPos; // View position (x, y, z). w is unused.
};
//------------------------------------------------------------------------------

// The skyview LUT to convolve.
layout(binding = 0) uniform sampler2DArray skyViewLUT;

// The output irradiance map.
layout(rgba16f, binding = 0) restrict writeonly uniform imageCubeArray radianceMap;

layout(std140, binding = 0) uniform HillaireParams
{
  AtmosphereParams u_atmoParams[MAX_NUM_ATMOSPHERES];
};

layout(std140, binding = 1) uniform IBLParams
{
  vec4 u_iblParams; // Roughness (x), number of diffuse importance samples (y),
};                  // number of specular importance samples (z). w is unused.

layout(std430, binding = 0) readonly buffer Indices
{
  int atmosphereIndices[MAX_NUM_ATMOSPHERES];
  int iblIndices[MAX_NUM_DYN_SKY_IBL];
};

// Function for converting between image coordiantes and world coordiantes.
vec3 cubeToWorld(ivec3 cubeCoord, vec2 cubeSize);

// Sample the sky view LUT.
vec3 sampleSkyViewLUT(float atmIndex, sampler2DArray lut, vec3 viewPos, vec3 viewDir,
                      vec3 sunDir, float groundRadiusMM);

// Smith-Schlick-Beckmann geometry function.
float SSBGeometry(vec3 N, vec3 H, float roughness);

// Hammersley sequence, generates a low discrepancy pseudorandom number.
vec2 Hammersley(uint i, uint N);

// Importance sampling of the Smith-Schlick-Beckmann geometry function.
vec3 SSBImportance(vec2 Xi, vec3 N, float roughness);

void main()
{
  const ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);

  const int slice = int(invoke.z) / 6;
  const int cubeFace = int(invoke.z) - (6 * slice);
  const int iblIndex = iblIndices[slice];
  const int atmoIndex = atmosphereIndices[slice];
  const AtmosphereParams params = u_atmoParams[atmoIndex];

  vec2 mipSize = vec2(imageSize(radianceMap).xy);
  ivec3 cubeCoord = ivec3(invoke.xy, cubeFace);
  vec3 worldPos = cubeToWorld(ivec3(invoke.xy, cubeFace), mipSize);

  vec3 sunDir = normalize(-params.sunDirAtmRadius.xyz);
  vec3 viewPos = vec3(params.viewPos.xyz);
  float groundRadiusMM = params.planetAlbedoRadius.w;

  // The normal is the same as the worldspace position.
  vec3 normal = normalize(worldPos);

  vec3 prefilteredColor = vec3(0.0);
  float totalWeight = 0.0;
  const uint sampleCount = uint(u_iblParams.z);
  for (uint i = 0u; i < sampleCount; i++)
  {
    vec2 Xi = Hammersley(i, sampleCount);
    vec3 H = SSBImportance(Xi, normal, u_iblParams.x);
    vec3 L  = normalize(2.0 * dot(normal, H) * H - normal);

    float NdotL = max(dot(normal, L), 0.0);
    if (NdotL > 0.0)
    {
      //L *= -1.0; // I do not know why this is necessary...
      vec3 skySample = sampleSkyViewLUT(float(atmoIndex), skyViewLUT, viewPos,
                                        L, sunDir, groundRadiusMM);
      prefilteredColor += skySample * NdotL;
      totalWeight += NdotL;
    }
  }

  prefilteredColor = prefilteredColor / totalWeight;

  imageStore(radianceMap, ivec3(invoke.xy, 6 * iblIndex + cubeFace), vec4(prefilteredColor, 1.0));
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

vec3 sampleSkyViewLUT(float atmIndex, sampler2DArray lut, vec3 viewPos, vec3 viewDir,
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

  return texture(lut, vec3(u, v, atmIndex)).rgb;
}

// Smith-Schlick-Beckmann geometry function.
float SSBGeometry(vec3 N, vec3 H, float roughness)
{
  float a      = roughness * roughness;
  float a2     = a * a;
  float NdotH  = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float nom   = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom       = PI * denom * denom;

  return nom / denom;
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

// Importance sampling of the Smith-Schlick-Beckmann geometry function.
vec3 SSBImportance(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space H vector to world-space sample vector
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}
