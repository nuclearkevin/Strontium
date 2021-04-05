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
  // The comment below is purely for debugging.
  //fragColour = vec4(textureLod(skybox, fragIn.fTexCoords, 1.2).rgb, 1.0);
}
