#type compute
#version 460 core
/*
 * Compute shader to generate the multi-scattering approximation lookup texture
 * as described in [Hillaire2020].
 * https://sebh.github.io/publications/egsr2020.pdf
 * https://github.com/sebh/UnrealEngineSkyAtmosphere
 * https://www.shadertoy.com/view/slSXRW
*/

#define PI 3.141592654

#define MS_IMAGE_SIZE 32
#define MAX_NUM_ATMOSPHERES 8
#define MULTISCATTER_STEPS 20
#define SQURT_SAMPLES 8

// x is the current texel in the LUT [0, 1024] -> [0, 32] x [0, 32]. y is the
// current atmosphere [0, 8]. z is the current scattering integral point [0, 64].
layout(local_size_x = 1, local_size_y = 1, local_size_z = 64) in;

// Atmospheric scattering coefficients are expressed per unit km.
struct ScatteringParams
{
  vec4 rayleighScat; //  Rayleigh scattering base (x, y, z) and height falloff (w).
  vec4 rayleighAbs; //  Rayleigh absorption base (x, y, z) and height falloff (w).
  vec4 mieScat; //  Mie scattering base (x, y, z) and height falloff (w).
  vec4 mieAbs; //  Mie absorption base (x, y, z) and height falloff (w).
  vec4 ozoneAbs; //  Ozone absorption base (x, y, z) and scale (w).
};

// Radii are expressed in megameters (MM).
struct AtmosphereParams
{
  ScatteringParams sParams;
  vec4 planetAlbedoRadius; // Planet albedo (x, y, z) and radius.
  vec4 sunDirAtmRadius; // Sun direction (x, y, z) and atmosphere radius (w).
  vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  vec4 viewPos; // View position (x, y, z). w is unused.
};

struct ScatteringResult
{
  vec3 lum;
  vec3 lumFactor;
};

layout(rgba16f, binding = 0) restrict writeonly uniform image2DArray multiScatImages;

layout(binding = 0) uniform sampler2DArray transLUTs;

layout(std140, binding = 0) uniform HillaireParams
{
  AtmosphereParams u_params[MAX_NUM_ATMOSPHERES];
};

layout(std430, binding = 1) readonly buffer HillaireIndices
{
  int atmosphereIndices[MAX_NUM_ATMOSPHERES];
};

// Compute a 2D index from a 1D index.
ivec2 imageIndex(int linearIndex)
{
  int y = linearIndex / MS_IMAGE_SIZE;
  return ivec2(linearIndex - y * MS_IMAGE_SIZE, y);
}

// Helper functions.
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

// Computes a single point of the multiple scattering approximation.
ScatteringResult computeSingleScat(float atmIndex, vec3 pos, vec3 rayDir, vec3 sunDir,
                                   ScatteringParams params, float groundRadiusMM,
                                   float atmosphereRadiusMM, vec3 groundAlbedo);

// Using parallel sums to compute the scattering integral.
shared vec3 fMs[64];
shared vec3 lMs[64];

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec2 texel = imageIndex(invoke.x);
  vec2 uv = (vec2(texel) + 0.5.xx) / vec2(imageSize(multiScatImages).xy);

  const int atmosphereIndex = atmosphereIndices[invoke.y];
  const AtmosphereParams params = u_params[atmosphereIndex];

  float groundRadiusMM = params.planetAlbedoRadius.w;
  float atmosphereRadiusMM = params.sunDirAtmRadius.w;
  vec3 groundAlbedo = params.planetAlbedoRadius.rgb;

  float sunCosTheta = 2.0 * uv.x - 1.0;
  float sunTheta = safeACos(sunCosTheta);
  float height = mix(groundRadiusMM, atmosphereRadiusMM, uv.y);

  vec3 pos = vec3(0.0, height, 0.0);
  vec3 sunDir = normalize(vec3(0.0, sunCosTheta, -sin(sunTheta)));

  float sqrtSample = float(SQURT_SAMPLES);
	float i = 0.5 + float(invoke.z / SQURT_SAMPLES);
	float j = 0.5 + float(invoke.z - float((invoke.z / SQURT_SAMPLES) * SQURT_SAMPLES));
  float phi = safeACos(1.0 - 2.0 * (float(j) + 0.5) / sqrtSample);
  float theta = PI * (float(i) + 0.5) / sqrtSample;
  vec3 rayDir = getSphericalDir(theta, phi);

  ScatteringResult result = computeSingleScat(float(atmosphereIndex), pos, rayDir, sunDir,
                                              params.sParams, groundRadiusMM,
                                              atmosphereRadiusMM, groundAlbedo);

  fMs[invoke.z] = result.lumFactor / (sqrtSample * sqrtSample);
  lMs[invoke.z] = result.lum / (sqrtSample * sqrtSample);
  barrier();

  // Parallel sums for an array of 64 elements.
  // 64 to 32
  if (invoke.z < 32)
  {
    fMs[invoke.z] += fMs[invoke.z + 32];
    lMs[invoke.z] += lMs[invoke.z + 32];
  }
  barrier();

  // 32 to 16
  if (invoke.z < 16)
  {
    fMs[invoke.z] += fMs[invoke.z + 16];
    lMs[invoke.z] += lMs[invoke.z + 16];
  }
  barrier();

  // 16 to 8.
  if (invoke.z < 8)
  {
    fMs[invoke.z] += fMs[invoke.z + 8];
    lMs[invoke.z] += lMs[invoke.z + 8];
  }
  barrier();

  if (invoke.z < 4)
  {
    fMs[invoke.z] += fMs[invoke.z + 4];
    lMs[invoke.z] += lMs[invoke.z + 4];
  }
  barrier();

  if (invoke.z < 2)
  {
    fMs[invoke.z] += fMs[invoke.z + 2];
    lMs[invoke.z] += lMs[invoke.z + 2];
  }
  barrier();

  if (invoke.z < 1)
  {
    fMs[invoke.z] += fMs[invoke.z + 1];
    lMs[invoke.z] += lMs[invoke.z + 1];
  }
  barrier();

  if (invoke.z > 0)
    return;

  vec3 psi = lMs[0] / (vec3(1.0) - fMs[0]);
  imageStore(multiScatImages, ivec3(texel, atmosphereIndex), vec4(max(psi, vec3(0.0)), 1.0));
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

vec3 getValFromLUT(float atmIndex, sampler2DArray tex, vec3 pos, vec3 sunDir,
                   float groundRadiusMM, float atmosphereRadiusMM)
{
  float height = length(pos);
  vec3 up = pos / height;

  float sunCosZenithAngle = dot(sunDir, up);

  float u = clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0);
  float v = max(0.0, min(1.0, (height - groundRadiusMM) / (atmosphereRadiusMM - groundRadiusMM)));

  return texture(tex, vec3(u, v, atmIndex)).rgb;
}

vec3 computeExtinction(vec3 pos, ScatteringParams params, float groundRadiusMM)
{
  float altitudeKM = (length(pos) - groundRadiusMM) * 1000.0;

  float rayleighDensity = exp(-altitudeKM / params.rayleighScat.w);
  float mieDensity = exp(-altitudeKM / params.mieScat.w);

  vec3 rayleighScattering = params.rayleighScat.rgb * rayleighDensity;
  vec3 rayleighAbsorption = params.rayleighAbs.rgb * rayleighDensity;

  vec3 mieScattering = params.mieScat.rgb * mieDensity;
  vec3 mieAbsorption = params.mieAbs.rgb * mieDensity;

  vec3 ozoneAbsorption = params.ozoneAbs.w * params.ozoneAbs.rgb * max(0.0, 1.0 - abs(altitudeKM - 25.0) / 15.0);

  return rayleighScattering + vec3(rayleighAbsorption + mieScattering + mieAbsorption) + ozoneAbsorption;
}

vec3 computeRayleighScattering(vec3 pos, ScatteringParams params, float groundRadiusMM)
{
  float altitudeKM = (length(pos) - groundRadiusMM) * 1000.0;
  float rayleighDensity = exp(-altitudeKM / params.rayleighScat.w);

  return params.rayleighScat.rgb * rayleighDensity;
}

vec3 computeMieScattering(vec3 pos, ScatteringParams params, float groundRadiusMM)
{
  float altitudeKM = (length(pos) - groundRadiusMM) * 1000.0;
  float mieDensity = exp(-altitudeKM / params.mieScat.w);

  return params.mieScat.rgb * mieDensity;
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

// Computes a single point of the multiple scattering approximation.
ScatteringResult computeSingleScat(float atmIndex, vec3 pos, vec3 rayDir,
                                   vec3 sunDir, ScatteringParams params,
                                   float groundRadiusMM, float atmosphereRadiusMM,
                                   vec3 groundAlbedo)
{
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
  for (uint stepI = 0; stepI < MULTISCATTER_STEPS; stepI++)
  {
    float newT = ((float(stepI) + 0.3) / float(MULTISCATTER_STEPS)) * tMax;
    float dt = newT - t;
    t = newT;

    vec3 newPos = pos + t * rayDir;

    vec3 rayleighScattering = computeRayleighScattering(newPos, params, groundRadiusMM);
    vec3 extinction = computeExtinction(newPos, params, groundRadiusMM);
    vec3 mieScattering = computeMieScattering(newPos, params, groundRadiusMM);

    vec3 sampleTransmittance = exp(-dt * extinction);

    vec3 scatteringNoPhase = rayleighScattering + vec3(mieScattering);
    vec3 scatteringF = (scatteringNoPhase - scatteringNoPhase * sampleTransmittance) / extinction;
    lumFactor += transmittance * scatteringF;

    vec3 sunTransmittance = getValFromLUT(atmIndex, transLUTs, newPos, sunDir,
                                          groundRadiusMM, atmosphereRadiusMM);

    vec3 rayleighInScattering = rayleighScattering * rayleighPhaseValue;
    vec3 mieInScattering = mieScattering * miePhaseValue;
    vec3 inScattering = (rayleighInScattering + mieInScattering) * sunTransmittance;

    // Integrated scattering within path segment.
    vec3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

    lum += scatteringIntegral * transmittance;
    transmittance *= sampleTransmittance;
  }

  // Bounced lighting from the ground using simple Lambertian diffuse.
  if (groundDist > 0.0)
  {
    vec3 hitPos = pos + groundDist * rayDir;
    if (dot(pos, sunDir) > 0.0)
    {
      const vec3 normal = normalize(hitPos);
      const float nDotL = clamp(dot(normal, sunDir), 0.0, 1.0);
      hitPos = normal * groundRadiusMM;
      vec3 sunTransmittance = getValFromLUT(atmIndex, transLUTs, hitPos, sunDir,
                                            groundRadiusMM, atmosphereRadiusMM);
      lum += transmittance * groundAlbedo * sunTransmittance * nDotL / PI;
    }
  }

  ScatteringResult results;
  results.lum = lum;
  results.lumFactor = lumFactor;

  return results;
}
