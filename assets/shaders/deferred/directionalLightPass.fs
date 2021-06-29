#version 440
/*
* Lighting fragment shader for a deferred PBR pipeline. Computes the directional
* component.
*/

#define PI 3.141592654
#define MAX_MIP 4.0
#define NUM_CASCADES 3
#define NUM_BLOCKER_SEARCHES 16
#define NUM_PCF_SAMPLES 32

// This is nasty.
vec2 poissonDisk[64] = vec2[]
(
  vec2(-0.613392, 0.617481),
  vec2(0.170019, -0.040254),
  vec2(-0.299417, 0.791925),
  vec2(0.645680, 0.493210),
  vec2(-0.651784, 0.717887),
  vec2(0.421003, 0.027070),
  vec2(-0.817194, -0.271096),
  vec2(-0.705374, -0.668203),
  vec2(0.977050, -0.108615),
  vec2(0.063326, 0.142369),
  vec2(0.203528, 0.214331),
  vec2(-0.667531, 0.326090),
  vec2(-0.098422, -0.295755),
  vec2(-0.885922, 0.215369),
  vec2(0.566637, 0.605213),
  vec2(0.039766, -0.396100),
  vec2(0.751946, 0.453352),
  vec2(0.078707, -0.715323),
  vec2(-0.075838, -0.529344),
  vec2(0.724479, -0.580798),
  vec2(0.222999, -0.215125),
  vec2(-0.467574, -0.405438),
  vec2(-0.248268, -0.814753),
  vec2(0.354411, -0.887570),
  vec2(0.175817, 0.382366),
  vec2(0.487472, -0.063082),
  vec2(-0.084078, 0.898312),
  vec2(0.488876, -0.783441),
  vec2(0.470016, 0.217933),
  vec2(-0.696890, -0.549791),
  vec2(-0.149693, 0.605762),
  vec2(0.034211, 0.979980),
  vec2(0.503098, -0.308878),
  vec2(-0.016205, -0.872921),
  vec2(0.385784, -0.393902),
  vec2(-0.146886, -0.859249),
  vec2(0.643361, 0.164098),
  vec2(0.634388, -0.049471),
  vec2(-0.688894, 0.007843),
  vec2(0.464034, -0.188818),
  vec2(-0.440840, 0.137486),
  vec2(0.364483, 0.511704),
  vec2(0.034028, 0.325968),
  vec2(0.099094, -0.308023),
  vec2(0.693960, -0.366253),
  vec2(0.678884, -0.204688),
  vec2(0.001801, 0.780328),
  vec2(0.145177, -0.898984),
  vec2(0.062655, -0.611866),
  vec2(0.315226, -0.604297),
  vec2(-0.780145, 0.486251),
  vec2(-0.371868, 0.882138),
  vec2(0.200476, 0.494430),
  vec2(-0.494552, -0.711051),
  vec2(0.612476, 0.705252),
  vec2(-0.578845, -0.768792),
  vec2(-0.772454, -0.090976),
  vec2(0.504440, 0.372295),
  vec2(0.155736, 0.065157),
  vec2(0.391522, 0.849605),
  vec2(-0.620106, -0.328104),
  vec2(0.789239, -0.419965),
  vec2(-0.545396, 0.538133),
  vec2(-0.178564, -0.596057)
);

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
uniform sampler2D cascadeMaps[NUM_CASCADES];

// Uniforms for the geometry buffer.
uniform vec2 screenSize;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMatProp;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

// Trowbridge-Reitz distribution function.
float TRDistribution(vec3 N, vec3 H, float alpha);
// Smith-Schlick-Beckmann geometry function.
float SSBGeometry(vec3 N, vec3 L, vec3 V, float roughness);
// Schlick approximation to the Fresnel factor.
vec3 SFresnel(float cosTheta, vec3 F0);
// Schlick approximation to the Fresnel factor, with roughness!
vec3 SFresnelR(float cosTheta, vec3 F0, float roughness);

// Calculate if the fragment is in shadow or not.
float calcShadow(uint cascadeIndex, vec3 position, vec3 normal, vec3 lightDir);

// Functions for softening shadows.
float calcPCFFactor(vec3 shadowCoords, uint cascadeIndex, float radius, float bias);

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
  vec3 F = SFresnel(max(dot(halfWay, view), 0.0), F0);

  vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

  vec3 num = NDF * G * F;
  float den = 4.0 * max(dot(normal, view), 0.0) * max(dot(normal, light), 0.0);
  vec3 spec = num / max(den, 0.001);

  vec4 clipSpacePos = camera.cameraView * vec4(position, 1.0);
  float shadowFactor = 1.0;

  for (uint i = 0; i < NUM_CASCADES; i++)
  {
    if (clipSpacePos.z > -cascadeSplits[i])
    {
      shadowFactor = calcShadow(i, position, normal, light);
      break;
    }
  }

  fragColour = vec4(shadowFactor * (kD * albedo / PI + spec) * lColour * lIntensity * max(dot(normal, light), 0.0), 1.0);
}

// Trowbridge-Reitz distribution function.
float TRDistribution(vec3 N, vec3 H, float roughness)
{
  float alpha = roughness * roughness;
  float a2 = alpha * alpha;
  float NdotH = max(dot(N, H), 0.0);
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
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float g2 = Geometry(NdotV, roughness);
  float g1 = Geometry(NdotL, roughness);

  return g1 * g2;
}

// Schlick approximation to the Fresnel factor.
vec3 SFresnel(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 SFresnelR(float cosTheta, vec3 F0, float roughness)
{
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// Calculate if the fragment is in shadow or not.
float calcShadow(uint cascadeIndex, vec3 position, vec3 normal, vec3 lightDir)
{
  vec4 lightClipPos = lightVP[cascadeIndex] * vec4(position, 1.0);
  vec3 projCoords = lightClipPos.xyz / lightClipPos.w;
  projCoords = 0.5 * projCoords + 0.5;

  float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

  return calcPCFFactor(projCoords, cascadeIndex, 1.0, bias);
}


float calcPCFFactor(vec3 shadowCoords, uint cascadeIndex, float radius, float bias)
{
  float sum = 0.0;
  ivec2 shadowSize = textureSize(cascadeMaps[cascadeIndex], 0);
  vec2 texelSize = 1.0 / vec2(shadowSize.x, shadowSize.y);

  float z = 0.0;
  for (uint i = 0; i < NUM_PCF_SAMPLES; i++)
  {
    z = texture(cascadeMaps[cascadeIndex], shadowCoords.xy + poissonDisk[i] * texelSize * radius).r;
    sum += shadowCoords.z - bias < z ? 1.0 : 0.0;
  }

  return sum / NUM_PCF_SAMPLES;
}
