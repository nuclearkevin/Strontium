#version 440
/*
 * Lighting fragment shader for experimenting with PBR lighting
 * techniques. Uses textures for material properties.
 */
#define PI 3.141592654
#define MAX_MIP 4.0
#define MAX_NUM_UNIFORM_LIGHTS 8
#define MAX_NUM_POINT_LIGHTS   8
#define MAX_NUM_SPOT_LIGHTS    8

// Different light types.
// A uniform light field. 20 float components.
struct UniformLight
{
  vec4 colour;
  vec4 direction;
  vec4 diffuse;
  vec4 specular;
  vec2 attenuation;
  float shininess;
  float intensity;
};
// A point light source. 20 float components.
struct PointLight
{
  vec4 colour;
  vec4 position;
  vec4 diffuse;
  vec4 specular;
  vec2 attenuation;
  float shininess;
  float intensity;
};
// A conical spotlight. 26 float components.
// cosTheta is inner cutoff, cosGamma is outer cutoff.
struct SpotLight
{
  vec4 colour;
  vec4 position;
  vec4 direction;
  vec4 diffuse;
  vec4 specular;
  vec2 attenuation;
  float shininess;
  float intensity;
  float cosTheta;
  float cosGamma;
};

struct FragMaterial
{
	vec3 albedo;
	float metallic;
	float roughness;
	float aOcclusion;
  vec3 position;
	vec3 normal;
  vec3 F0;
};

// The camera position in worldspace. 6 float components.
struct Camera
{
  vec3 position;
  vec3 viewDir;
};

in VERT_OUT
{
	vec3 fNormal;
	vec3 fPosition;
	vec3 fColour;
  vec2 fTexCoords;
	mat3 fTBN;
} fragIn;

// Input lights.
layout(std140, binding = 1) uniform uLightCollection
{
  UniformLight uLight[MAX_NUM_UNIFORM_LIGHTS];
};
layout(std140, binding = 2) uniform pLightCollection
{
  PointLight pLight[MAX_NUM_POINT_LIGHTS];
};
layout(std140, binding = 3) uniform sLightCollection
{
  SpotLight sLight[MAX_NUM_SPOT_LIGHTS];
};

uniform uint numULights;
uniform uint numPLights;
uniform uint numSLights;

// Camera uniform.
uniform Camera camera;

// Uniforms for PBR textures.
uniform float roughness = 0.1;
uniform float metallic = 0.8;
uniform float aOcclusion = 1.0;

// Uniforms for ambient lighting.
uniform samplerCube irradianceMap;
uniform samplerCube reflectanceMap;
uniform sampler2D brdfLookUp;

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

// Lighting function declarations.
vec3 computeUL(UniformLight light, FragMaterial frag, vec3 viewDir);
vec3 computePL(PointLight light, FragMaterial frag, vec3 viewDir);
vec3 computeSL(SpotLight light, FragMaterial frag, vec3 viewDir);

// Main function.
void main()
{
	FragMaterial frag;
  frag.albedo = pow(fragIn.fColour.rgb, vec3(2.2));
	frag.normal = normalize(fragIn.fNormal);
	frag.metallic = metallic;
	frag.roughness = roughness;
	frag.aOcclusion = aOcclusion;
  frag.position = fragIn.fPosition;
  frag.F0 = mix(vec3(0.04), frag.albedo, frag.metallic);

	vec3 view = normalize(camera.position - fragIn.fPosition);
  vec3 reflection = reflect(-view, frag.normal);

  vec3 ks = SFresnelR(max(dot(frag.normal, view), 0.0), frag.F0, frag.roughness);
  vec3 kd = (vec3(1.0) - ks) * (1.0 - frag.roughness);

	vec3 radiosity = vec3(0.0);
	vec3 ambientDiff = kd * texture(irradianceMap, frag.normal).rgb * frag.albedo;
  vec3 ambientSpec = textureLod(reflectanceMap, reflection,
                                frag.roughness * MAX_MIP).rgb;
  vec2 brdfInt = texture(brdfLookUp, vec2(max(dot(frag.normal, view), 0.0),
                                          frag.roughness)).rg;
  ambientSpec = ambientSpec * (brdfInt.r * ks + brdfInt.g);

	for (int i = 0; i < numULights; i++)
    radiosity += computeUL(uLight[i], frag, view);
  for (int i = 0; i < numPLights; i++)
    radiosity += computePL(pLight[i], frag, view);
  for (int i = 0; i < numSLights; i++)
    radiosity += computeSL(sLight[i], frag, view);

	vec3 colour = (radiosity + (ambientDiff + ambientSpec) * frag.aOcclusion);
	// HDR correct.
	colour = colour / (colour + vec3(1.0));
	// Tone map, gamma correction.
	colour = pow(colour, vec3(1.0 / 2.2));

  fragColour = vec4(colour, 1.0);
}

// Trowbridge-Reitz distribution function.
float TRDistribution(vec3 N, vec3 H, float roughness)
{
	float alpha = roughness*roughness;
  float a2 = alpha * alpha;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH*NdotH;

  float nom   = a2;
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

// Implementation of the lighting functions.
vec3 computeUL(UniformLight light, FragMaterial frag, vec3 viewDir)
{
	// Setup the properties of the light.
	vec3 lightDir = normalize(-light.direction.xyz);
	vec3 halfwayDir = normalize(viewDir + lightDir);
	vec3 radiance = light.colour.rgb * light.intensity;

	// Cook-Torrance BRDF model.
	float NDF = TRDistribution(frag.normal, halfwayDir, frag.roughness);
	float G   = SSBGeometry(frag.normal, viewDir, lightDir, frag.roughness);
	vec3 F    = SFresnel(max(dot(halfwayDir, viewDir), 0.0), frag.F0);

	// Assemble the radiance contribution.
	vec3 ks = F;
	vec3 kd = vec3(1.0) - ks;
	kd 		  *= (1.0 - frag.metallic);

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(frag.normal, viewDir), 0.0) * max(dot(frag.normal, lightDir), 0.0);
	vec3 specular = numerator / max(denominator, 0.001);
	float NdotL = max(dot(frag.normal, lightDir), 0.0);
	return (kd * frag.albedo / PI + specular) * radiance * NdotL;
}

vec3 computePL(PointLight light, FragMaterial frag, vec3 viewDir)
{
	// Setup the properties of the light.
	vec3 lightDir = normalize(light.position.xyz - frag.position);
	vec3 halfwayDir = normalize(viewDir + lightDir);
	float attenuation = 1 / (1.0 + length(light.position.xyz - frag.position) * light.attenuation.x
                      + (length(light.position.xyz - frag.position)
                      * length(light.position.xyz - frag.position)) * light.attenuation.y);
	vec3 radiance = attenuation * light.colour.rgb * light.intensity;

	// Cook-Torrance BRDF model.
	float NDF = TRDistribution(frag.normal, halfwayDir, frag.roughness);
  float G = SSBGeometry(frag.normal, viewDir, lightDir, frag.roughness);
  vec3 F = SFresnel(max(dot(halfwayDir, viewDir), 0.0), frag.F0);

	// Assemble the radiance contribution.
	vec3 ks = F;
	vec3 kd = vec3(1.0) - ks;
	kd *= (1.0 - frag.metallic);

	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(frag.normal, viewDir), 0.0) * max(dot(frag.normal, lightDir), 0.0);
	vec3 specular     = numerator / max(denominator, 0.001);
	float NdotL 			= max(dot(frag.normal, lightDir), 0.0);
	return (kd * frag.albedo / PI + specular) * radiance * NdotL;
}

vec3 computeSL(SpotLight light, FragMaterial frag, vec3 viewDir)
{
	return vec3(0.0);
}
