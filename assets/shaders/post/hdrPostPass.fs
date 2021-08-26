#version 440

/*
 * The general post-processing shader. Composites bloom and performs tone mapping.
*/

// The post processing properties.
layout(std140, binding = 0) uniform PostProcessBlock
{
  mat4 u_invViewProj;
  mat4 u_viewProj;
  vec4 u_camPosScreenSize; // Camera position (x, y, z) and the screen width (w).
  vec4 u_screenSizeGammaBloom;  // Screen height (x), gamma (y) and bloom intensity (z). w is unused.
  ivec4 u_postProcessingPasses; // Tone mapping operator (x). y-w unused.
};

layout(binding = 0) uniform sampler2D screenColour;
layout(binding = 1) uniform sampler2D entityIDs;
layout(binding = 2) uniform sampler2D bloomColour;

// Output colour variable.
layout(location = 1) out vec4 fragColour;
layout(location = 0) out float fragID;

// Helper functions.
float rgbToLuminance(vec3 rgbColour);

// Bloom.
vec3 blendBloom(vec2 bloomTexCoords, sampler2D bloomTexture, float bloomIntensity);

// Tone mapping.
vec3 toneMap(vec3 colour, uint operator);
vec3 reinhardOperator(vec3 rgbColour);
vec3 luminanceReinhardOperator(vec3 rgbColour);
vec3 luminanceReinhardJodieOperator(vec3 rgbColour);
vec3 partialUnchartedOperator(vec3 rgbColour);
vec3 filmicUnchartedOperator(vec3 rgbColour);
vec3 fastAcesOperator(vec3 rgbColour);
vec3 acesOperator(vec3 rgbColour);

// Gamma correct.
vec3 applyGamma(vec3 colour, float gamma);

void main()
{
  vec2 screenSize = vec2(u_camPosScreenSize.w, u_screenSizeGammaBloom.x);
  vec2 fTexCoords = gl_FragCoord.xy / screenSize;

  vec3 colour = texture(screenColour, fTexCoords).rgb;
  if (u_postProcessingPasses.y != 0)
    colour += blendBloom(fTexCoords, bloomColour, u_screenSizeGammaBloom.z);

  colour = toneMap(colour, uint(u_postProcessingPasses.x));
  colour = applyGamma(colour, u_screenSizeGammaBloom.y);

  fragColour = vec4(colour, 1.0);
  fragID = texture(entityIDs, fTexCoords).a;
}

// Helper functions.
float rgbToLuminance(vec3 rgbColour)
{
  return dot(rgbColour, vec3(0.2126f, 0.7152f, 0.0722f));
}

// Bloom.
vec3 blendBloom(vec2 bloomTexCoords, sampler2D bloomTexture, float bloomIntensity)
{
  return texture(bloomTexture, bloomTexCoords).rgb * bloomIntensity;
}

// Tone mapping.
vec3 toneMap(vec3 colour, uint operator)
{
  switch (operator)
  {
    case 0: return reinhardOperator(colour);
    case 1: return luminanceReinhardOperator(colour);
    case 2: return luminanceReinhardJodieOperator(colour);
    case 3: return filmicUnchartedOperator(colour);
    case 4: return fastAcesOperator(colour);
    case 5: return acesOperator(colour);
    default: return colour;
  }
}

vec3 reinhardOperator(vec3 rgbColour)
{
  return rgbColour / (vec3(1.0) + rgbColour);
}

vec3 luminanceReinhardOperator(vec3 rgbColour)
{
  float luminance = rgbToLuminance(rgbColour);

  return rgbColour / (1.0 + luminance);
}

vec3 luminanceReinhardJodieOperator(vec3 rgbColour)
{
  float luminance = rgbToLuminance(rgbColour);
  vec3 tv = rgbColour / (vec3(1.0) + rgbColour);

  return mix(rgbColour / (1.0 + luminance), tv, tv);
}

vec3 partialUnchartedOperator(vec3 rgbColour)
{
  const float a = 0.15;
  const float b = 0.50;
  const float c = 0.10;
  const float d = 0.20;
  const float e = 0.02;
  const float f = 0.30;

  return ((rgbColour * (a * rgbColour + c * b) + d * e)
          / (rgbColour * (a * rgbColour + b) + d * f)) - e / f;
}

vec3 filmicUnchartedOperator(vec3 rgbColour)
{
  float bias = 2.0;
  vec3 current = partialUnchartedOperator(bias * rgbColour);

  vec3 white = vec3(11.2);
  vec3 whiteScale = vec3(1.0) / partialUnchartedOperator(white);

  return current * whiteScale;
}

vec3 fastAcesOperator(vec3 rgbColour)
{
  rgbColour *= 0.6;
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;

  return clamp((rgbColour * (a * rgbColour + b))
               / (rgbColour * (c * rgbColour + d) + e), 0.0f, 1.0f);
}

vec3 acesOperator(vec3 rgbColour)
{
  const mat3 inputMatrix = mat3
  (
    vec3(0.59719, 0.07600, 0.02840),
    vec3(0.35458, 0.90834, 0.13383),
    vec3(0.04823, 0.01566, 0.83777)
  );

  const mat3 outputMatrix = mat3
  (
    vec3(1.60475, -0.10208, -0.00327),
    vec3(-0.53108, 1.10813, -0.07276),
    vec3(-0.07367, -0.00605, 1.07602)
  );

  vec3 inputColour = inputMatrix * rgbColour;
  vec3 a = inputColour * (inputColour + vec3(0.0245786)) - vec3(0.000090537);
  vec3 b = inputColour * (0.983729 * inputColour + 0.4329510) + 0.238081;
  vec3 c = a / b;
  return outputMatrix * c;
}

// Gamma correct.
vec3 applyGamma(vec3 colour, float gamma)
{
  return pow(colour, vec3(1.0 / gamma));
}
