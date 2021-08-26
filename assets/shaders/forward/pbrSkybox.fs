#version 440

/*
 * A generic forward pass IBL skybox shader. Shares a buffer block with the
 * Preetham skybox shader.
*/

#define MAX_MIP 4.0

layout(binding = 0) uniform samplerCube skybox;

layout(std140, binding = 1) uniform SkyboxBlock
{
  vec4 u_lodDirection; // IBL lod (x), sun direction (y, z, w).
  float u_turbidity;
};

in VERT_OUT
{
  vec3 fTexCoords;
} fragIn;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

void main()
{
  vec3 envColor = textureLod(skybox, fragIn.fTexCoords, u_lodDirection.x * MAX_MIP).rgb;
  fragColour = vec4(envColor, 1.0);
}
