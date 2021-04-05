#version 440
/*
 * A fragment shader to compute the prefilter component of the split-sum
 * approach for environmental specular lighting.
*/

#define PI 3.141592654

layout(location = 0) out vec4 fragColour;

in vec3 fPosition;

uniform samplerCube environmentMap;

uniform float roughness;
uniform float resolution;

// Smith-Schlick-Beckmann geometry function.
float SSBGeometry(vec3 N, vec3 H, float roughness);
// Hammersley sequence pseudorandom number generator.
vec2 Hammersley(uint i, uint N);
// Importance sampling of the Smith-Schlick-Beckmann geometry function.
vec3 SSBImportance(vec2 Xi, vec3 N, float roughness);

// Total number of samples.
const uint SAMPLE_COUNT = 1024u;

void main()
{
  vec3 N = normalize(fPosition);

  vec3 prefilteredColor = vec3(0.0);
  float totalWeight = 0.0;

  for(uint i = 0u; i < SAMPLE_COUNT; ++i)
  {
    vec2 Xi = Hammersley(i, SAMPLE_COUNT);
    vec3 H = SSBImportance(Xi, N, roughness);
    vec3 L  = normalize(2.0 * dot(N, H) * H - N);

    float NdotL = max(dot(N, L), 0.0);
    if(NdotL > 0.0)
    {
      float D   = SSBGeometry(N, H, roughness);
      float NdotH = max(dot(N, H), 0.0);
      float HdotV = max(dot(H, N), 0.0);
      float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

      //float resolution = 512.0;
      float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
      float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

      float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

      prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
      totalWeight      += NdotL;
    }
  }

  prefilteredColor = prefilteredColor / totalWeight;

  fragColour = vec4(prefilteredColor, 1.0);
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

// Hammersley sequence pseudorandom number generator.
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
