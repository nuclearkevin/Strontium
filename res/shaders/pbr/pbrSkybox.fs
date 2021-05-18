#version 440

#define MAX_MIP 4.0

uniform samplerCube skybox;

uniform float gamma = 2.2;
uniform float exposure = 1.0;

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
  envColor = envColor / (envColor + vec3(exposure));
  fragColour = vec4(pow(envColor, vec3(1.0 / gamma)), 1.0);
}
