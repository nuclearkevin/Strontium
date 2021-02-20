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
  vec4 position;
  float intensity;
  LightMaterial mat;
};
// A conical spotlight. 23 float components.
// cosTheta is inner cutoff, cosGamma is outer cutoff.
struct SpotLight
{
  vec3 colour;
  vec4 position;
  vec3 direction;
  float intensity;
  float cosTheta;
  float cosGamma;
  LightMaterial mat;
};

// The camera position in worldspace. 7 float components.
struct Camera
{
  vec4 position;
  vec3 viewDir;
};

// Vertex properties for shading.
in vec3 fNormal;
in vec4 fPosition;
in vec3 fColour;

uniform uint numULights;
uniform uint numPLights;
uniform uint numSLights;

uniform vec4 ambientColour;

// Input lights.
uniform UniformLight uLight[MAX_NUM_UNIFORM_LIGHTS];
uniform PointLight   pLight[MAX_NUM_POINT_LIGHTS];
uniform SpotLight    sLight[MAX_NUM_SPOT_LIGHTS];

// Camera uniform.
uniform Camera camera;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

// Function declarations.
vec3 computeUL(UniformLight light, vec3 normal, vec4 position, Camera camera);
vec3 computePL(PointLight light, vec3 normal, vec4 position, Camera camera);
vec3 computeSL(SpotLight light, vec3 normal, vec4 position, Camera camera);

void main()
{
  // Completed light model. Loops over each type of light and adds its
  // contribution to the fragment.
  vec3 result = vec3(0.0, 0.0, 0.0);
  result += ambientColour.rgb * 0.1;

  for (int i = 0; i < numULights; i++)
    result += computeUL(uLight[i], fNormal, fPosition, camera);
  for (int i = 0; i < numPLights; i++)
    result += computePL(pLight[i], fNormal, fPosition, camera);
  for (int i = 0; i < numSLights; i++)
    result += computeSL(sLight[i], fNormal, fPosition, camera);

	fragColour = vec4(clamp(result * fColour, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0)), 1.0);
}

// Compute the lighting contribution from a uniform light field.
vec3 computeUL(UniformLight light, vec3 normal, vec4 position, Camera camera)
{
  // Uniform light lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float diff = max(dot(normalize(normal), light.direction), 0.0);
  float spec = pow(max(dot(vec3(normalize(position - camera.position)),
              reflect(normalize(light.direction), normalize(normal))), 0.0), 32)
              * 0.5;
  return light.intensity * light.colour * (diff + spec);
}

vec3 computePL(PointLight light, vec3 normal, vec4 position, Camera camera)
{
  // Point light lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  vec3 ray = normalize(position - light.position).xyz;
  float attenuation = 1 / (1.0 + length(light.position - position) * 0.14
                      + (length(light.position - position)
                      * length(light.position - position)) * 0.07);
  float diff = max(dot(normalize(normal), ray), 0.0);
  float spec = pow(max(dot(vec3(normalize(position - camera.position)),
              reflect(ray, normalize(normal))), 0.0), 32)
              * 0.5;
  return light.intensity * light.colour * attenuation * (diff + spec);
}

// Computes the lighting contribution from a spotlight.
vec3 computeSL(SpotLight light, vec3 normal, vec4 position, Camera camera)
{
  vec3 ray = normalize(position - light.position).xyz;
  float phi = dot(ray, normalize(-light.direction));

  // Spotlight lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float radialAtten = clamp((phi - light.cosGamma) /
                            (light.cosTheta - light.cosGamma), 0.0, 1.0);
  float attenuation = 1 / (1.0 + length(light.position - position) * 0.14
                      + (length(light.position - position)
                      * length(light.position - position)) * 0.07);
  float diff = max(dot(normalize(normal), light.direction), 0.0);
  float spec = pow(max(dot(vec3(normalize(position - camera.position)),
              reflect(normalize(light.direction), normalize(normal))), 0.0), 32)
              * 0.5;
  return light.intensity * light.colour * radialAtten * attenuation * (diff + spec);
}
