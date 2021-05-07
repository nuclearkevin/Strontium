#version 440

uniform samplerCube skybox;

uniform float gamma = 2.2;
uniform vec3 exposure = vec3(1.0);

in VERT_OUT
{
  vec3 fTexCoords;
} fragIn;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

void main()
{
  vec3 envColor = texture(skybox, fragIn.fTexCoords).rgb;
  envColor = envColor / (envColor + exposure);
  fragColour = vec4(pow(envColor, vec3(1.0 / gamma)), 1.0);
}
