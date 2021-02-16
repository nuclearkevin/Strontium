#version 440 core
/*
 *  A very simple fragment shader which implements Phong shading.
 */

// Vertex normal.
in vec3 normal;
in vec4 position;

// Skybox light source direction vector, camera position.
uniform vec3 skybox;
uniform vec4 camera;

// Output colour variable.
out vec4 FragColour;

void main()
{
  // White light, object colour is red.
  vec3 lightColour = vec3(1.0, 1.0, 1.0);
  vec3 colour = vec3(1.0, 0.0, 0.0);

  // Ambient light.
  vec3 ambient = 0.15 * lightColour;

  // Diffusive light.
	vec3 norm = normalize(normal);
	float diff = max(dot(norm ,skybox), 0.0);
  vec3 diffuse = diff * lightColour;

  // Specular lighting.
  vec3 viewDir = vec3(normalize(camera - position));
  float specularStrength = 0.5;
  vec3 reflectDir = reflect(-skybox, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
  vec3 specular = specularStrength * spec * lightColour;

  // Completed light model.
  vec3 result = (ambient + diff + specular) * colour;
	FragColour = vec4(result, 1.0);
}
