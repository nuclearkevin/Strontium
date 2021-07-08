#version 440
/*
* Lighting fragment shader for a deferred PBR pipeline. Computes the directional
* component.
*/

#define PI 3.141592654
#define THRESHHOLD 0.05
#define NUM_CASCADES 4
#define NUM_BLOCKER_SEARCHES 16
#define NUM_PCF_SAMPLES 32

struct Camera
{
 vec3 position;
 vec3 viewDir;
 mat4 cameraView;
};

// Camera uniform.
uniform Camera camera;

// Directional light uniforms.
uniform vec3 lDirection;
uniform vec3 lColour;
uniform float lIntensity;

// Shadow map uniforms.
uniform mat4 lightVP[NUM_CASCADES];
uniform float cascadeSplits[NUM_CASCADES];
uniform float shadowTuning[NUM_CASCADES];
layout(binding = 7) uniform sampler2D cascadeMaps[NUM_CASCADES];

// Uniforms for the geometry buffer.
uniform vec2 screenSize;
layout(binding = 3) uniform sampler2D gPosition;
layout(binding = 4) uniform sampler2D gNormal;
layout(binding = 5) uniform sampler2D gAlbedo;
layout(binding = 6) uniform sampler2D gMatProp;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

//------------------------------------------------------------------------------
// Janky LearnOpenGL PBR.
//------------------------------------------------------------------------------
// Trowbridge-Reitz distribution function.
float TRDistribution(vec3 N, vec3 H, float alpha);
// Smith-Schlick-Beckmann geometry function.
float SSBGeometry(vec3 N, vec3 L, vec3 V, float roughness);
// Schlick approximation to the Fresnel factor.
vec3 SFresnel(float cosTheta, vec3 F0);
// Schlick approximation to the Fresnel factor, with roughness!
vec3 SFresnelR(float cosTheta, vec3 F0, float roughness);

//------------------------------------------------------------------------------
// Shadow calculations. Cascaded exponential shadow mapping!
//------------------------------------------------------------------------------
// Calculate if the fragment is in shadow or not.
float calcShadow(uint cascadeIndex, vec3 position, vec3 normal, vec3 lightDir);

void main()
{
  vec2 fTexCoords = gl_FragCoord.xy / screenSize;

  vec3 position = texture(gPosition, fTexCoords).xyz;
  vec3 normal = normalize(texture(gNormal, fTexCoords).xyz);
  vec3 albedo = texture(gAlbedo, fTexCoords).rgb;
  float metallic = texture(gMatProp, fTexCoords).r;
  float roughness = texture(gMatProp, fTexCoords).g;
  float ao = texture(gMatProp, fTexCoords).b;

  vec3 F0 = mix(vec3(0.04), albedo, metallic);

  vec3 view = normalize(position - camera.position);
  vec3 light = normalize(lDirection);
  vec3 halfWay = normalize(view + light);

  float NDF = TRDistribution(normal, halfWay, roughness);
  float G = SSBGeometry(normal, view, light, roughness);
  vec3 F = SFresnel(max(dot(halfWay, view), THRESHHOLD), F0);

  vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

  vec3 num = NDF * G * F;
  float den = 4.0 * max(dot(normal, view), THRESHHOLD) * max(dot(normal, light), THRESHHOLD);
  vec3 spec = num / max(den, THRESHHOLD);

  vec4 clipSpacePos = camera.cameraView * vec4(position, 1.0);
  float shadowFactor = 1.0;

  for (uint i = 0; i < NUM_CASCADES; i++)
  {
    if (clipSpacePos.z > -(cascadeSplits[i]))
    {
      shadowFactor = calcShadow(i, position, normal, light);
      break;
    }
  }

  fragColour = vec4(shadowFactor * (kD * albedo / PI + spec) * lColour * lIntensity * max(dot(normal, light), THRESHHOLD), 1.0);
}

// Trowbridge-Reitz distribution function.
float TRDistribution(vec3 N, vec3 H, float roughness)
{
  float alpha = roughness * roughness;
  float a2 = alpha * alpha;
  float NdotH = max(dot(N, H), THRESHHOLD);
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / denom;
}

// Schlick-Beckmann geometry function.
float Geometry(float NdotV, float roughness)
{
  float r = (roughness + 1.0);
  float k = (roughness * roughness) / 8.0;

  float nom   = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return nom / denom;
}

// Smith's modified geometry function.
float SSBGeometry(vec3 N, vec3 L, vec3 V, float roughness)
{
  float NdotV = max(dot(N, V), THRESHHOLD);
  float NdotL = max(dot(N, L), THRESHHOLD);
  float g2 = Geometry(NdotV, roughness);
  float g1 = Geometry(NdotL, roughness);

  return g1 * g2;
}

// Schlick approximation to the Fresnel factor.
vec3 SFresnel(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, THRESHHOLD), 5.0);
}

vec3 SFresnelR(float cosTheta, vec3 F0, float roughness)
{
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, THRESHHOLD), 5.0);
}

// Calculate if the fragment is in shadow or not, than applies exponential shadow mapping.
float calcShadow(uint cascadeIndex, vec3 position, vec3 normal, vec3 lightDir)
{
  vec4 lightClipPos = lightVP[cascadeIndex] * vec4(position, 1.0);
  vec3 projCoords = lightClipPos.xyz / lightClipPos.w;
  projCoords = 0.5 * projCoords + 0.5;

  float expOccDepth = texture(cascadeMaps[cascadeIndex], projCoords.xy).r;
  float expRecDepth = exp(-1.0 * shadowTuning[cascadeIndex] * projCoords.z);
  float shadowFactor = clamp(expOccDepth * expRecDepth, 0.0, 1.0);

  return projCoords.z > 1.0 ? 1.0 : shadowFactor;
}
