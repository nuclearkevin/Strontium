#version 440

/*
 * The general post-processing shader. Composites bloom and performs tone mapping
 * using the Reinhard operator.
*/

// The post processing properties.
layout(std140, binding = 0) uniform PostProcessBlock
{
  mat4 u_invViewProj;
  mat4 u_viewProj;
  vec4 u_camPosScreenSize; // Camera position (x, y, z) and the screen width (w).
  vec3 u_screenSizeGammaBloom;  // Screen height (x), gamma (y) and bloom intensity (z).
};

layout(binding = 0) uniform sampler2D screenColour;
layout(binding = 1) uniform sampler2D entityIDs;
layout(binding = 2) uniform sampler2D bloomColour;

// Output colour variable.
layout(location = 1) out vec4 fragColour;
layout(location = 0) out float fragID;

void main()
{
  vec2 screenSize = vec2(u_camPosScreenSize.w, u_screenSizeGammaBloom.x);
  float gamma = u_screenSizeGammaBloom.y;
  float bloomIntensity = u_screenSizeGammaBloom.z;

  vec2 bloomTexSize = vec2(textureSize(bloomColour, 0).xy);
  vec2 fTexCoords = gl_FragCoord.xy / screenSize;
  vec2 bloomTexCoords = gl_FragCoord.xy / bloomTexSize;

  vec3 colour = texture(screenColour, fTexCoords).rgb;
  colour += texture(bloomColour, bloomTexCoords).rgb * bloomIntensity;

  colour = colour / (colour + vec3(1.0));
  colour = pow(colour, vec3(1.0 / gamma));
  fragColour = vec4(colour, 1.0);
  fragID = texture(entityIDs, fTexCoords).a;
}
