#type compute
#version 460 core
/*
 * A compute shader to compute the prefilter component of the split-sum
 * approach for environmental specular lighting. Modified to sample the sky
 * view LUT of Hillaire2020.
 * Many thanks to http://alinloghin.com/articles/compute_ibl.html, some of the
 * code here was adapted from their excellent article.
*/

#define TWO_PI 6.283185308
#define PI 3.141592654
#define PI_OVER_TWO 1.570796327

layout(local_size_x = 8, local_size_y = 8) in;

// The environment map to prefilter.
layout(binding = 0) uniform sampler2D skyviewLUT;

// The output prefilter map.
layout(rgba16f, binding = 1) restrict writeonly uniform imageCube prefilterMap;

// The required parameters.
layout(std140, binding = 2) buffer ParamBuff
{
  vec4 u_iblParams; // Roughness (x). y, z and w are unused.
  ivec4 u_iblParams1; // Number of samples (x). y, z and w are unused.
};

// Skybox specific uniforms
layout(std140, binding = 3) uniform SkyboxBlock
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
                      vec3 sunDir, float groundRadiusMM, float mipLevel);
// Smith-Schlick-Beckmann geometry function.
float SSBGeometry(vec3 N, vec3 H, float roughness);
// Hammersley sequence, generates a low discrepancy pseudorandom number.
vec2 Hammersley(uint i, uint N);
// Importance sampling of the Smith-Schlick-Beckmann geometry function.
vec3 SSBImportance(vec2 Xi, vec3 N, float roughness);

void main()
{
  const uint sampleCount = u_iblParams1.x;
  vec2 mipSize = vec2(imageSize(prefilterMap).xy);
  ivec3 cubeCoord = ivec3(gl_GlobalInvocationID);

  vec3 sunPos = normalize(u_lodDirection.yzw);
  vec3 viewPos = u_viewPosSkyIntensity.xyz;
  float groundRadiusMM =  u_sunIntensitySizeGRadiusARadius.z;

  vec3 worldPos = cubeToWorld(cubeCoord, mipSize);
  vec3 N = normalize(worldPos);

  vec3 prefilteredColor = vec3(0.0);
  float totalWeight = 0.0;

  for (uint i = 0u; i < sampleCount; ++i)
  {
    vec2 Xi = Hammersley(i, sampleCount);
    vec3 H = SSBImportance(Xi, N, u_iblParams.x);
    vec3 L  = normalize(2.0 * dot(N, H) * H - N);

    float NdotL = max(dot(N, L), 0.0);
    if (NdotL > 0.0)
    {
      float D = SSBGeometry(N, H, u_iblParams.x);
      float NdotH = max(dot(N, H), 0.0);
      float HdotV = max(dot(H, N), 0.0);
      float pdf = D * NdotH / (4.0 * HdotV + 0.0001);

      float saTexel  = 4.0 * PI / (6.0 * 256.0 * 128.0);
      float saSample = 1.0 / (float(sampleCount) * pdf + 0.0001);

      float mipLevel = u_iblParams.x == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

      vec3 skySample = sampleSkyViewLUT(skyviewLUT, viewPos, L, sunPos,
                                        groundRadiusMM, mipLevel);
      prefilteredColor += u_viewPosSkyIntensity.w * skySample * NdotL;
      totalWeight += NdotL;
    }
  }

  prefilteredColor = prefilteredColor / totalWeight;

  imageStore(prefilterMap, cubeCoord, vec4(prefilteredColor, 1.0));
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
                      vec3 sunDir, float groundRadiusMM, float mipLevel)
{
  viewDir.xz *= -1.0;
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

  return textureLod(lut, vec2(u, v), mipLevel).rgb;
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
