#version 440

uniform vec2 screenSize;
uniform float gamma = 2.2;
uniform sampler2D screenColour;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

void main()
{
  vec2 fTexCoords = gl_FragCoord.xy / screenSize;

  vec3 colour = texture(screenColour, fTexCoords).rgb;

  colour = colour / (colour + vec3(1.0));
  colour = pow(colour, vec3(1.0 / gamma));
  fragColour = vec4(colour, 1.0);
}
