#version 440
/*
 * The fragment shader for the light sources.
 */

// Colour for drawing the light sphere.
uniform vec3 colour;

layout(location = 0) out vec4 fragColour;

void main()
{
  fragColour = vec4(colour, 1.0);
}
