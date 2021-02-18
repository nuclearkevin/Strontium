#version 440 core
/*
 * Lighting fragment shader for experimenting with rasterized lighting
 * techniques.
 */

 struct Material {
     vec3 ambient;
     vec3 diffuse;
     vec3 specular;
     float shininess;
 };

struct SpotLight
{
  vec3 colour;
  vec4 position;
  vec3 direction;

  float intensity;
  // cosTheta is inner cutoff, cosGamma is outer.
  float cosTheta;
  float cosGamma;
};

// Vertex normal and position.
in vec3 normal;
in vec4 position;
in vec3 colour;

// Spotlight.
uniform SpotLight light;

// Camera properties.
uniform SpotLight camera;

// Output colour variable.
out vec4 FragColour;

// Function declarations.
vec3 computeSL(SpotLight light, vec3 normal, vec4 position, vec4 camPos);

void main()
{
  // White ambient.
  vec3 ambientColour = vec3(1.0, 1.0, 1.0);

  vec3 ambient = ambientColour * 0.1;

  // Completed light model.
  vec3 result = vec3(0.0, 0.0, 0.0);
  result = ambient + computeSL(light, normal, position, camera.position);
	FragColour = vec4(clamp(result * colour, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0)), 1.0);
}

// Computes the lighting contribution from a spotlight.
vec3 computeSL(SpotLight light, vec3 normal, vec4 position, vec4 camPos)
{
  vec3 ray = normalize(position - light.position).xyz;
  float phi = dot(ray, normalize(-light.direction));

  // Spotlight lighting calculations. Yes, its spaghetti. ( ͡° ͜ʖ ͡°).
  float radialAtten = clamp((phi - light.cosGamma) /
                            (light.cosTheta - light.cosGamma), 0.0, 1.0);
  float attenuation = 1 / (1.0 + length(light.position - position) * 0.14
                      + (length(light.position - position)
                      * length(light.position - position)) * 0.07);
  vec3 diff = max(dot(normalize(normal), light.direction), 0.0)
              * light.intensity * light.colour;
  vec3 spec = pow(max(dot(vec3(normalize(position - camPos)),
              reflect(light.direction, normalize(normal))), 0.0), 32)
              * light.intensity * light.colour * 0.5;
  return radialAtten * attenuation * (diff + spec);
}
