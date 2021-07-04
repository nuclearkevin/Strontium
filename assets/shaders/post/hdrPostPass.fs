#version 440

uniform vec2 screenSize;
uniform float gamma = 2.2;

layout(binding = 0) uniform sampler2D screenColour;
layout(binding = 1) uniform sampler2D entityIDs;

// Output colour variable.
layout(location = 1) out vec4 fragColour;
layout(location = 0) out float fragID;

void main()
{
  vec2 fTexCoords = gl_FragCoord.xy / screenSize;

  vec3 colour = texture(screenColour, fTexCoords).rgb;

  colour = colour / (colour + vec3(1.0));
  colour = pow(colour, vec3(1.0 / gamma));
  fragColour = vec4(colour, 1.0);
  fragID = texture(entityIDs, fTexCoords).a;
}
