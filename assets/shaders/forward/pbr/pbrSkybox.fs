#version 440

#define MAX_MIP 4.0

uniform samplerCube skybox;

uniform float roughness = 0.0;

in VERT_OUT
{
  vec3 fTexCoords;
} fragIn;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

void main()
{
  vec3 envColor = textureLod(skybox, fragIn.fTexCoords, roughness * MAX_MIP).rgb;
  fragColour = vec4(envColor, 1.0);
}
