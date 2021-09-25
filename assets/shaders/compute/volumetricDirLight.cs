#version 440

/*
 * A volumetric directional light compute shader. Computes sunbeams at half
 * resolution with some dithering.
*/

#define PI 3.141592654

#define NUM_STEPS 50

#define NUM_CASCADES 4
#define WARP 44.0

layout(local_size_x = 32, local_size_y = 32) in;

// The output texture for the volumetric effect.
layout(rgba16f, binding = 0) writeonly uniform image2D volumetric;

layout(binding = 3) uniform sampler2D gPosition;
layout(binding = 7) uniform sampler2D cascadeMaps[NUM_CASCADES]; // TODO: Texture arrays.

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  vec3 u_camPosition;
};

// Directional light uniforms.
layout(std140, binding = 5) uniform DirectionalBlock
{
  vec4 u_lColourIntensity;
  vec4 u_lDirection;
  vec2 u_screenSize;
};

layout(std140, binding = 7) uniform CascadedShadowBlock
{
  mat4 u_lightVP[NUM_CASCADES];
  float u_cascadeSplits[NUM_CASCADES];
  float u_lightBleedReduction;
};

layout(std140, binding = 0) buffer LightShaftParams
{
  vec4 u_mieScatIntensity; // Mie scattering coefficients (x, y, z), light shaft intensity (w).
  vec4 u_mieAbsDensity; // Mie scattering coefficients (x, y, z), density (w).
};

// Helper functions.
float sampleDither(ivec2 coords);

// Mie phase function.
float getMiePhase(float cosTheta);

//------------------------------------------------------------------------------
// Shadow calculations. Cascaded exponential variance shadow mapping!
//------------------------------------------------------------------------------
float calcShadow(uint cascadeIndex, vec3 position);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);

  // Half resolution.
  ivec2 gBufferCoords = 2 * invoke;
  vec2 gBufferSize = textureSize(gPosition, 0).xy;
  vec2 gBufferUV = vec2(gBufferCoords) / gBufferSize;

  // Light properties.
  vec3 lightDir = normalize(u_lDirection.xyz);
  vec3 lightColour = u_lColourIntensity.xyz;
  float lightIntensity = u_lColourIntensity.w;
  float volumetricIntensity = u_mieScatIntensity.w;

  // Participating medium properties.
  float density = u_mieAbsDensity.w;
  vec3 mieScattering = density * u_mieScatIntensity.xyz * 0.01;
  vec3 mieAbsorption = density * u_mieAbsDensity.xyz * 0.01;
  vec3 extinction = (mieScattering + mieAbsorption);

  // March from the camera position to the fragment position.
  vec3 endPos = texture(gPosition, gBufferUV).rgb;
  vec3 startPos = u_camPosition.xyz;
  vec3 ray = endPos - startPos;
  vec3 rayDir = normalize(ray);
  vec3 rayStep = ray / float(NUM_STEPS);
  float rayStepLength = length(rayStep);

  // Dither to add some noise.
  vec3 currentPos = startPos + (rayStep * sampleDither(gBufferCoords));

  vec3 fog = vec3(0.0);
  vec3 totalTransmittance = vec3(1.0);
  float miePhaseValue = getMiePhase(dot(lightDir, rayDir));

  // Integrate the light contribution and transmittance along the ray.
  for (uint i = 0; i < NUM_STEPS; i++)
  {
    float shadowFactor;
    vec4 clipSpacePos = u_viewMatrix * vec4(currentPos, 1.0);

    // Cascaded shadow maps.
    for (uint i = 0; i < NUM_CASCADES; i++)
    {
      if (clipSpacePos.z > -(u_cascadeSplits[i]))
      {
        shadowFactor = calcShadow(i, currentPos);
        break;
      }
    }

    vec3 sampleTransmittance = exp(-rayStepLength * extinction);
    totalTransmittance *= sampleTransmittance;

    vec3 inScattering = mieScattering * miePhaseValue;
    vec3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

    fog += scatteringIntegral * totalTransmittance * lightColour * lightIntensity * shadowFactor;

    currentPos += rayStep;
  }

  fog *= volumetricIntensity;
  fog = max(fog, vec3(0.0));
  imageStore(volumetric, invoke, vec4(fog, 1.0));
}

float sampleDither(ivec2 coords)
{
  const mat4 ditherMatrix = mat4
  (
    vec4(0.0, 0.5, 0.125, 0.625),
    vec4(0.75, 0.22, 0.875, 0.375),
    vec4(0.1875, 0.6875, 0.0625, 0.5625),
    vec4(0.9375, 0.4375, 0.8125, 0.3125)
  );

  return ditherMatrix[coords.x % 4][coords.y % 4];
}

float getMiePhase(float cosTheta)
{
  const float g = 0.8;
  const float scale = 3.0 / (8.0 * PI);

  float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
  float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
}

vec2 warpDepth(float depth)
{
  float posWarp = exp(WARP * depth);
  float negWarp = -1.0 * exp(-1.0 * WARP * depth);
  return vec2(posWarp, negWarp);
}

float computeChebyshevBound(float moment1, float moment2, float depth)
{
  float variance2 = moment2 - moment1 * moment1;
  float diff = depth - moment1;
  float diff2 = diff * diff;
  float pMax = clamp((variance2 / (variance2 + diff2) - u_lightBleedReduction) / (1.0 - u_lightBleedReduction), 0.0, 1.0);

  return moment1 < depth ? pMax : 1.0;
}

// Calculate if the fragment is in shadow or not, than shadow mapping.
float calcShadow(uint cascadeIndex, vec3 position)
{
  vec4 lightClipPos = u_lightVP[cascadeIndex] * vec4(position, 1.0);
  vec3 projCoords = lightClipPos.xyz / lightClipPos.w;
  projCoords = 0.5 * projCoords + 0.5;

  vec4 moments = texture(cascadeMaps[cascadeIndex], projCoords.xy).rgba;
  vec2 warpedDepth = warpDepth(projCoords.z);

  float shadowFactor1 = computeChebyshevBound(moments.r, moments.g, warpedDepth.r);
  float shadowFactor2 = computeChebyshevBound(moments.b, moments.a, warpedDepth.g);
  float shadowFactor = min(shadowFactor1, shadowFactor2);

  return shadowFactor;
}
