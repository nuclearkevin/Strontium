#version 440

/*
 * Computes the multi-scattering approximation lookup texture as described in
 * [Hillaire2020].
 * https://sebh.github.io/publications/egsr2020.pdf
 *
 * Based off of a shadertoy implementation by Andrew Helmer:
 * https://www.shadertoy.com/view/slSXRW
*/

#define PI 3.141592654

#define MULTISCATTER_STEPS 20.0
#define SQURT_SAMPLES 8

layout(local_size_x = 32, local_size_y = 32) in;

layout(rgba16f, binding = 0) writeonly uniform image2D multiScatImage;

layout(binding = 2) uniform sampler2D transLUT;

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
vec3 getSphericalDir(float theta, float phi);

// Fetch a transmittance from the transmittance LUT.
vec3 getValFromTLUT(sampler2D tex, vec3 pos, vec3 sunDir, float groundRadiusMM,
                    float atmosphereRadiusMM);

// Compute scattering parameters.
vec3 computeExtinction(vec3 pos, ScatteringParams params, float groundRadiusMM);
vec3 computeRayleighScattering(vec3 pos, ScatteringParams params, float groundRadiusMM);
float computeMieScattering(vec3 pos, ScatteringParams params, float groundRadiusMM);

// Compute scattering phase functions.
float getMiePhase(float cosTheta);
float getRayleighPhase(float cosTheta);

// Compute the multi-scattering approximation.
vec3 computeMultiScat(vec3 pos, vec3 sunDir, ScatteringParams params,
                      float groundRadiusMM, float atmosphereRadiusMM);

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

  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 size = vec2(imageSize(multiScatImage).xy);
  vec2 coords = vec2(invoke);
  vec2 uv = coords / size;

  float sunCosTheta = 2.0 * uv.x - 1.0;
  float sunTheta = safeACos(sunCosTheta);
  float height = mix(groundRadiusMM, atmosphereRadiusMM, uv.y);

  vec3 pos = vec3(0.0, height, 0.0);
  vec3 sunDir = normalize(vec3(0.0, sunCosTheta, -sin(sunTheta)));

  vec3 psi = computeMultiScat(pos, sunDir, params, groundRadiusMM, atmosphereRadiusMM);

  imageStore(multiScatImage, invoke, vec4(max(psi, vec3(0.0)), 1.0));
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

vec3 getSphericalDir(float theta, float phi)
{
  float cosPhi = cos(phi);
  float sinPhi = sin(phi);
  float cosTheta = cos(theta);
  float sinTheta = sin(theta);
  return vec3(sinPhi * sinTheta, cosPhi, sinPhi * cosTheta);
}

vec3 getValFromTLUT(sampler2D tex, vec3 pos, vec3 sunDir,
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

vec3 computeMultiScat(vec3 pos, vec3 sunDir, ScatteringParams params,
                      float groundRadiusMM, float atmosphereRadiusMM)
{
  vec3 lumTotal = vec3(0.0);
  vec3 fms = vec3(0.0);

  float invSamples = 1.0 / float(SQURT_SAMPLES * SQURT_SAMPLES);
  // Integrate the second order scattering luminance over the sphere surrounding
  // the viewing position.
  for (int i = 0; i < SQURT_SAMPLES; i++)
  {
    for (int j = 0; j < SQURT_SAMPLES; j++)
    {
      float theta = PI * (float(i) + 0.5) / float(SQURT_SAMPLES);
      float phi = safeACos(1.0 - 2.0 * (float(j) + 0.5) / float(SQURT_SAMPLES));
      vec3 rayDir = getSphericalDir(theta, phi);

      float atmoDist = rayIntersectSphere(pos, rayDir, atmosphereRadiusMM);
      float groundDist = rayIntersectSphere(pos, rayDir, groundRadiusMM);
      float tMax = groundDist > 0.0 ? groundDist : atmoDist;

      float cosTheta = dot(rayDir, sunDir);

      float miePhaseValue = getMiePhase(cosTheta);
      float rayleighPhaseValue = getRayleighPhase(-cosTheta);

      vec3 lum = vec3(0.0);
      vec3 lumFactor = vec3(0.0);
      vec3 transmittance = vec3(1.0);
      float t = 0.0;
      for (float stepI = 0.0; stepI < MULTISCATTER_STEPS; stepI += 1.0)
      {
        float newT = ((stepI + 0.3) / MULTISCATTER_STEPS) * tMax;
        float dt = newT - t;
        t = newT;

        vec3 newPos = pos + t * rayDir;

        vec3 rayleighScattering = computeRayleighScattering(newPos, params, groundRadiusMM);
        vec3 extinction = computeExtinction(newPos, params, groundRadiusMM);
        float mieScattering = computeMieScattering(newPos, params, groundRadiusMM);

        vec3 sampleTransmittance = exp(-dt * extinction);

        vec3 scatteringNoPhase = rayleighScattering + vec3(mieScattering);
        vec3 scatteringF = (scatteringNoPhase - scatteringNoPhase * sampleTransmittance) / extinction;
        lumFactor += transmittance * scatteringF;

        vec3 sunTransmittance = getValFromTLUT(transLUT, newPos, sunDir, groundRadiusMM, atmosphereRadiusMM);

        vec3 rayleighInScattering = rayleighScattering * rayleighPhaseValue;
        float mieInScattering = mieScattering * miePhaseValue;
        vec3 inScattering = (rayleighInScattering + mieInScattering) * sunTransmittance;

        // Integrated scattering within path segment.
        vec3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

        lum += scatteringIntegral * transmittance;
        transmittance *= sampleTransmittance;
      }

      if (groundDist > 0.0)
      {
        vec3 hitPos = pos + groundDist * rayDir;
        if (dot(pos, sunDir) > 0.0)
        {
          hitPos = normalize(hitPos) * groundRadiusMM;
          // TODO: Change the vec3(0.3) to a buffered parameter for customization.
          lum += transmittance * vec3(0.3) * getValFromTLUT(transLUT, hitPos, sunDir, groundRadiusMM, atmosphereRadiusMM);
        }
      }

      fms += lumFactor * invSamples;
      lumTotal += lum * invSamples;
    }
  }

  vec3 psi = lumTotal / (vec3(1.0) - fms);
  return psi;
}
