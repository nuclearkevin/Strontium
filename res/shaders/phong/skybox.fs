#version 440

uniform samplerCube skybox;

in VERT_OUT
{
  vec3 fTexCoords;
} fragIn;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

void main()
{
  fragColour = texture(skybox, fragIn.fTexCoords);
}
