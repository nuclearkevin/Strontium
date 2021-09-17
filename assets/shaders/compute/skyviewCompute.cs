#version 440

/*
 * Computes the sky-view lookup texture as described in
 * [Hillaire2020].
 * https://sebh.github.io/publications/egsr2020.pdf
 *
 * Based off of a shadertoy implementation by Andrew Helmer:
 * https://www.shadertoy.com/view/slSXRW
*/

#define PI 3.141592654

#define NUM_STEPS 32

layout(local_size_x = 32, local_size_y = 32) in;

layout(rgba16f, binding = 0) writeonly uniform image2D skyviewImage;

layout(binding = 2) uniform sampler2D transLUT;
layout(binding = 3) uniform sampler2D multiScatLut;

// TODO: Convert Mie scattering, Mie absorption and Rayleigh absorption to vec3s
// for more control. Add in density parameters for Mie, Rayleigh and Ozone
// coefficients. Add in a planet albedo factor. Add in a light colour and
// intensity factor.
layout(std140, binding = 1) buffer HillaireParams
{
  vec4 u_rayleighScatAbsBase; // Rayleigh scattering base (x, y, z) and the Rayleigh absorption base (w).
  vec4 u_ozoneAbsBaseMieScatBase; // Ozone absorption base (x, y, z) and the Mie scattering base (w).
  vec4 u_mieAbsBaseGradiusAradiusViewX; // Mie absorption base (x), ground radius in megameters (y), atmosphere radius in megameters (z), x component of the view pos (w).
  vec4 u_sunDirViewY; // Sun direction (x, y, z) and the y component of the view position (w).
  vec4 u_viewPosZ; // Z component of the view position (x). y, z and w are unused.
};

// 2 * vec4 + 1 * float.
struct ScatteringParams
{
  vec3 rayleighScatteringBase;
  float rayleighAbsorptionBase;
  float mieScatteringBase;
  float mieAbsorptionBase;
  vec3 ozoneAbsorptionBase;
};

// Helper functions.
float rayIntersectSphere(vec3 ro, vec3 rd, float rad);
float safeACos(float x);

// Fetch a transmittance from the transmittance LUT.
vec3 getValFromLUT(sampler2D tex, vec3 pos, vec3 sunDir, float groundRadiusMM,
                   float atmosphereRadiusMM);

// Compute scattering parameters.
vec3 computeExtinction(vec3 pos, ScatteringParams params, float groundRadiusMM);
vec3 computeRayleighScattering(vec3 pos, ScatteringParams params, float groundRadiusMM);
float computeMieScattering(vec3 pos, ScatteringParams params, float groundRadiusMM);

// Compute scattering phase functions.
float getMiePhase(float cosTheta);
float getRayleighPhase(float cosTheta);

// Raymarch to compute the scattering integral.
vec3 raymarchScattering(vec3 pos, vec3 rayDir, vec3 sunDir, float tMax,
                        ScatteringParams params, float groundRadiusMM,
                        float atmoRadiusMM);

void main()
{
  ScatteringParams params;
  params.rayleighScatteringBase = u_rayleighScatAbsBase.xyz;
  params.rayleighAbsorptionBase = u_rayleighScatAbsBase.w;
  params.mieScatteringBase = u_ozoneAbsBaseMieScatBase.w;
  params.mieAbsorptionBase = u_mieAbsBaseGradiusAradiusViewX.x;
  params.ozoneAbsorptionBase = u_ozoneAbsBaseMieScatBase.xyz;

  float groundRadiusMM = u_mieAbsBaseGradiusAradiusViewX.y;
  float atmosphereRadiusMM = u_mieAbsBaseGradiusAradiusViewX.z;

  vec3 sunDir = normalize(u_sunDirViewY.xyz);
  vec3 viewPos = vec3(u_mieAbsBaseGradiusAradiusViewX.w, u_sunDirViewY.w, u_viewPosZ.x);

  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 size = vec2(imageSize(skyviewImage).xy);
  float u = invoke.x / size.x;
  float v = invoke.y / size.y;

  float azimuthAngle = 2.0 * PI * (u - 0.5);
  float adjV;
  if (v < 0.5)
  {
    float coord = 1.0 - 2.0 * v;
    adjV = -(coord * coord);
  }
  else
  {
    float coord = v * 2.0 - 1.0;
    adjV = coord * coord;
  }

  float height = length(viewPos);
  vec3 up = viewPos / height;
  float horizonAngle = safeACos(sqrt(height * height - groundRadiusMM * groundRadiusMM) / height) - 0.5 * PI;
  float altitudeAngle = adjV * 0.5 * PI - horizonAngle;

  float cosAltitude = cos(altitudeAngle);
  vec3 rayDir = normalize(vec3(cosAltitude * sin(azimuthAngle), sin(altitudeAngle), -cosAltitude * cos(azimuthAngle)));

  float sunAltitude = (0.5 * PI) - acos(dot(sunDir, up));
  vec3 newSunDir = normalize(vec3(0.0, sin(sunAltitude), -cos(sunAltitude)));

  float atmoDist = rayIntersectSphere(viewPos, rayDir, atmosphereRadiusMM);
  float groundDist = rayIntersectSphere(viewPos, rayDir, groundRadiusMM);
  float tMax = (groundDist < 0.0) ? atmoDist : groundDist;

  float cutoff = dot(-1.0 * up, sunDir);
  // Compares with the cos(30deg). Fixes a strange issue where the skybox gets
  // computed when the sun is close to directly below the viewer.
  float comp = float(cutoff < 0.154);

  vec3 lum = comp * raymarchScattering(viewPos, rayDir, newSunDir, tMax, params,
                                       groundRadiusMM, atmosphereRadiusMM);
  imageStore(skyviewImage, invoke, vec4(lum, 1.0));
}

float rayIntersectSphere(vec3 ro, vec3 rd, float rad)
{
  float b = dot(ro, rd);
  float c = dot(ro, ro) - rad * rad;
  if (c > 0.0f && b > 0.0)
    return -1.0;

  float discr = b * b - c;
  if (discr < 0.0)
    return -1.0;

  // Special case: inside sphere, use far discriminant
  if (discr > b * b)
    return (-b + sqrt(discr));

  return -b - sqrt(discr);
}

float safeACos(float x)
{
  return acos(clamp(x, -1.0, 1.0));
}

vec3 getValFromLUT(sampler2D tex, vec3 pos, vec3 sunDir,
                   float groundRadiusMM, float atmosphereRadiusMM)
{
  float height = length(pos);
  vec3 up = pos / height;

  float sunCosZenithAngle = dot(sunDir, up);

  float u = clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0);
  float v = max(0.0, min(1.0, (height - groundRadiusMM) / (atmosphereRadiusMM - groundRadiusMM)));

  return texture(tex, vec2(u, v)).rgb;
}

vec3 computeExtinction(vec3 pos, ScatteringParams params, float groundRadiusMM)
{
  float altitudeKM = (length(pos) - groundRadiusMM) * 1000.0;

  float rayleighDensity = exp(-altitudeKM / 8.0);
  float mieDensity = exp(-altitudeKM / 1.2);

  vec3 rayleighScattering = params.rayleighScatteringBase * rayleighDensity;
  float rayleighAbsorption = params.rayleighAbsorptionBase * rayleighDensity;

  float mieScattering = params.mieScatteringBase * mieDensity;
  float mieAbsorption = params.mieAbsorptionBase * mieDensity;

  vec3 ozoneAbsorption = params.ozoneAbsorptionBase * max(0.0, 1.0 - abs(altitudeKM - 25.0) / 15.0);

  return rayleighScattering + vec3(rayleighAbsorption + mieScattering + mieAbsorption) + ozoneAbsorption;
}

vec3 computeRayleighScattering(vec3 pos, ScatteringParams params, float groundRadiusMM)
{
  float altitudeKM = (length(pos) - groundRadiusMM) * 1000.0;
  float rayleighDensity = exp(-altitudeKM / 8.0);

  return params.rayleighScatteringBase * rayleighDensity;
}

float computeMieScattering(vec3 pos, ScatteringParams params, float groundRadiusMM)
{
  float altitudeKM = (length(pos) - groundRadiusMM) * 1000.0;
  float mieDensity = exp(-altitudeKM / 1.2);

  return params.mieScatteringBase * mieDensity;
}

float getMiePhase(float cosTheta)
{
  const float g = 0.8;
  const float scale = 3.0 / (8.0 * PI);

  float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
  float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
}

float getRayleighPhase(float cosTheta)
{
  const float k = 3.0 / (16.0 * PI);
  return k * (1.0 + cosTheta * cosTheta);
}

vec3 raymarchScattering(vec3 pos, vec3 rayDir, vec3 sunDir, float tMax,
                        ScatteringParams params, float groundRadiusMM,
                        float atmoRadiusMM)
{
  float cosTheta = dot(rayDir, sunDir);

  float miePhaseValue = getMiePhase(cosTheta);
  float rayleighPhaseValue = getRayleighPhase(-cosTheta);

  vec3 lum = vec3(0.0);
  vec3 transmittance = vec3(1.0);
  float t = 0.0;
  for (float i = 0.0; i < float(NUM_STEPS); i += 1.0)
  {
    float newT = ((i + 0.3) / float(NUM_STEPS)) * tMax;
    float dt = newT - t;
    t = newT;

    vec3 newPos = pos + t * rayDir;

    vec3 rayleighScattering = computeRayleighScattering(newPos, params, groundRadiusMM);
    vec3 extinction = computeExtinction(newPos, params, groundRadiusMM);
    float mieScattering = computeMieScattering(newPos, params, groundRadiusMM);

    vec3 sampleTransmittance = exp(-dt * extinction);

    vec3 sunTransmittance = getValFromLUT(transLUT, newPos, sunDir,
                                          groundRadiusMM, atmoRadiusMM);

    vec3 psiMS = getValFromLUT(multiScatLut, newPos, sunDir,
                               groundRadiusMM, atmoRadiusMM);

    vec3 rayleighInScattering = rayleighScattering * (rayleighPhaseValue * sunTransmittance + psiMS);
    vec3 mieInScattering = mieScattering * (miePhaseValue * sunTransmittance + psiMS);
    vec3 inScattering = (rayleighInScattering + mieInScattering);

    // Integrated scattering within path segment.
    vec3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

    lum += scatteringIntegral * transmittance;

    transmittance *= sampleTransmittance;
  }

  return lum;
}
