#version 440
/*
 * Lighting fragment shader for experimenting with rasterized lighting
 * techniques.
 */
#define MAX_NUM_UNIFORM_LIGHTS 8
#define MAX_NUM_POINT_LIGHTS   8
#define MAX_NUM_SPOT_LIGHTS    8

// Light interaction properties. 10 float components.
struct LightMaterial
{
  vec3 diffuse;
  vec3 specular;
  float shininess;

  vec2 attenuation;
 };

// Different light types.
// A uniform light field. 17 float components.
struct UniformLight
{
  vec3 colour;
  vec3 direction;
  float intensity;
  LightMaterial mat;
};
// A point light source. 18 float components.
struct PointLight
{
  vec3 colour;
  vec3 position;
  float intensity;
  LightMaterial mat;
};
// A conical spotlight. 23 float components.
// cosTheta is inner cutoff, cosGamma is outer cutoff.
struct SpotLight
{
  vec3 colour;
  vec3 position;
  vec3 direction;
  float intensity;
  float cosTheta;
  float cosGamma;
  LightMaterial mat;
};

// The camera position in worldspace. 7 float components.
struct Camera
{
  vec3 position;
  vec3 viewDir;
};

// Vertex properties for shading.
in VERT_OUT
{
  vec3 fNormal;
	vec3 fPosition;
	vec3 fColour;
} fragIn;

uniform uint numULights;
uniform uint numPLights;
uniform uint numSLights;

uniform vec3 ambientColour;

uniform vec2 falloff;

// Input lights.
uniform UniformLight uLight[MAX_NUM_UNIFORM_LIGHTS];
uniform PointLight   pLight[MAX_NUM_POINT_LIGHTS];
uniform SpotLight    sLight[MAX_NUM_SPOT_LIGHTS];

// Camera uniform.
uniform Camera camera;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

// Function declarations.
vec3 computeUL(UniformLight light, vec3 normal, vec3 position, vec3 colour, Camera camera);
vec3 computePL(PointLight light, vec3 normal, vec3 position, vec3 colour, Camera camera);
vec3 computeSL(SpotLight light, vec3 normal, vec3 position, vec3 colour, Camera camera);

void main()
{
  // Completed light model. Loops over each type of light and adds its
  // contribution to the fragment.
  vec3 result = vec3(0.0, 0.0, 0.0);
  result += ambientColour * 0.1;

  for (int i = 0; i < numULights; i++)
    result += computeUL(uLight[i], fragIn.fNormal, fragIn.fPosition, fragIn.fColour, camera);
  for (int i = 0; i < numPLights; i++)
    result += computePL(pLight[i], fragIn.fNormal, fragIn.fPosition, fragIn.fColour, camera);
  for (int i = 0; i < numSLights; i++)
    result += computeSL(sLight[i], fragIn.fNormal, fragIn.fPosition, fragIn.fColour, camera);

	fragColour = vec4(clamp(result, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0)), 1.0);
}

// Compute the lighting contribution from a uniform light field.
vec3 computeUL(UniformLight light, vec3 normal, vec3 position, vec3 colour, Camera camera)
{
  vec3 nNormal = normalize(normal);
  vec3 lightDir = normalize(-light.direction);
  vec3 viewDir = normalize(camera.position - position);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  // Uniform light lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float diff = max(dot(nNormal, lightDir), 0.0);
  float spec = pow(max(dot(normal, halfwayDir), 0.0), 64) * 0.5;

  return light.intensity * light.colour * (colour * diff + spec);
}

vec3 computePL(PointLight light, vec3 normal, vec3 position, vec3 colour, Camera camera)
{
  vec3 nNormal = normalize(normal);
  vec3 lightDir = -normalize(position - light.position);
  vec3 viewDir = normalize(camera.position - position);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  // Point light lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float attenuation = 1 / (1.0 + length(light.position - position) * falloff.x
                      + (length(light.position - position)
                      * length(light.position - position)) * falloff.y);
  float diff = max(dot(nNormal, lightDir), 0.0);
  float spec = pow(max(dot(normal, halfwayDir), 0.0), 64) * 0.5;

  return light.intensity * light.colour * attenuation * (colour * diff + spec);
}

// Computes the lighting contribution from a spotlight.
vec3 computeSL(SpotLight light, vec3 normal, vec3 position, vec3 colour, Camera camera)
{
  vec3 nNormal = normalize(normal);
  vec3 lightDir = -normalize(light.direction);
  vec3 viewDir = normalize(camera.position - position);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  vec3 ray = -normalize(position - light.position).xyz;
  float phi = dot(ray, normalize(-light.direction));

  // Spotlight lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float radialAtten = clamp((phi - light.cosGamma) /
                            (light.cosTheta - light.cosGamma), 0.0, 1.0);
  float attenuation = 1 / (1.0 + length(light.position - position) * falloff.x
                      + (length(light.position - position)
                      * length(light.position - position)) * falloff.y);
  float diff = max(dot(nNormal, lightDir), 0.0);
  float spec = pow(max(dot(normal, halfwayDir), 0.0), 64) * 0.5;

  return light.intensity * light.colour * radialAtten * attenuation * (colour * diff + spec);
}
