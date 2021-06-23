#version 440
/*
 * Lighting fragment shader for experimenting with rasterized lighting
 * techniques.
 */
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
  vec2 fTexCoords;
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

uniform vec3 ambientColour;

// Camera uniform.
uniform Camera camera;

// Object textures.
uniform sampler2D texMap;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

// Function declarations.
vec3 computeUL(UniformLight light, vec3 normal, vec3 position, vec3 colour, Camera camera);
vec3 computePL(PointLight light, vec3 normal, vec3 position, vec3 colour, Camera camera);
vec3 computeSL(SpotLight light, vec3 normal, vec3 position, vec3 colour, Camera camera);

void main()
{
  // Sample the texture.
  vec3 texColour = texture(texMap, fragIn.fTexCoords).rgb;

  // Completed light model. Loops over each type of light and adds its
  // contribution to the fragment.
  vec3 result = vec3(0.0, 0.0, 0.0);
  result += ambientColour * 0.1;

  for (int i = 0; i < numULights; i++)
    result += computeUL(uLight[i], fragIn.fNormal, fragIn.fPosition, texColour, camera);
  for (int i = 0; i < numPLights; i++)
    result += computePL(pLight[i], fragIn.fNormal, fragIn.fPosition, texColour, camera);
  for (int i = 0; i < numSLights; i++)
    result += computeSL(sLight[i], fragIn.fNormal, fragIn.fPosition, texColour, camera);

	fragColour = vec4(clamp(result, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0)), 1.0);
}

// Compute the lighting contribution from a uniform light field.
vec3 computeUL(UniformLight light, vec3 normal, vec3 position, vec3 colour, Camera camera)
{
  vec3 nNormal = normalize(normal);
  vec3 lightDir = normalize(-light.direction.xyz);
  vec3 viewDir = normalize(camera.position - position);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  // Uniform light lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float diff = max(dot(nNormal, lightDir), 0.0);
  float spec = pow(max(dot(normal, halfwayDir), 0.0), light.shininess) * 0.5;

  return light.intensity * light.colour.rgb *
         (light.diffuse.rbg * colour * diff + light.specular.rgb * spec);
}

vec3 computePL(PointLight light, vec3 normal, vec3 position, vec3 colour, Camera camera)
{
  vec3 nNormal = normalize(normal);
  vec3 lightDir = -normalize(position - light.position.xyz);
  vec3 viewDir = normalize(camera.position - position);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  // Point light lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float attenuation = 1 / (1.0 + length(light.position.xyz - position) * light.attenuation.x
                      + (length(light.position.xyz - position)
                      * length(light.position.xyz - position)) * light.attenuation.y);
  float diff = max(dot(nNormal, lightDir), 0.0);
  float spec = pow(max(dot(normal, halfwayDir), 0.0), light.shininess) * 0.5;

  return light.intensity * light.colour.rgb * attenuation *
         (light.diffuse.rgb * colour * diff + light.specular.rgb * spec);
}

// Computes the lighting contribution from a spotlight.
vec3 computeSL(SpotLight light, vec3 normal, vec3 position, vec3 colour, Camera camera)
{
  vec3 nNormal = normalize(normal);
  vec3 lightDir = -normalize(light.direction.xyz);
  vec3 viewDir = normalize(camera.position - position);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  vec3 ray = -normalize(position - light.position.xyz);
  float phi = dot(ray, normalize(-light.direction.xyz)); // I had no idea why this works, in reverse. . .

  // Spotlight lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float radialAtten = clamp((phi - light.cosGamma) /
                            (light.cosTheta - light.cosGamma), 0.0, 1.0);
  float attenuation = 1 / (1.0 + length(light.position.xyz - position) *
                      light.attenuation.x + (length(light.position.xyz - position)
                      * length(light.position.xyz - position)) * light.attenuation.y);
  float diff = max(dot(nNormal, lightDir), 0.0);
  float spec = pow(max(dot(normal, halfwayDir), 0.0), light.shininess) * 0.5;

  return light.intensity * light.colour.rgb * radialAtten * attenuation *
         (light.diffuse.rgb * colour * diff + light.specular.rgb * spec);
}
