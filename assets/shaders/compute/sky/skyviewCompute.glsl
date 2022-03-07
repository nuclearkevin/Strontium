#type compute
#version 460 core
/*
 * Compute shader to generate the sky-view lookup texture as described in
 * [Hillaire2020].
 * https://sebh.github.io/publications/egsr2020.pdf
 * https://github.com/sebh/UnrealEngineSkyAtmosphere
 * https://www.shadertoy.com/view/slSXRW
*/

#define MAX_NUM_ATMOSPHERES 8
#define NUM_STEPS 32

#define PI 3.141592654

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

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

layout(rgba16f, binding = 0) restrict writeonly uniform image2DArray skyviewImages;

layout(binding = 0) uniform sampler2DArray transLUTs;
layout(binding = 1) uniform sampler2DArray multiScatLUTs;

layout(std140, binding = 0) uniform HillaireParams
{
  AtmosphereParams u_params[MAX_NUM_ATMOSPHERES];
};

layout(std430, binding = 1) readonly buffer HillaireIndices
{
  int u_atmosphereIndices[MAX_NUM_ATMOSPHERES];
};

// Helper functions.
float safeACos(float x)
{
  return acos(clamp(x, -1.0, 1.0));
}

// Ray-sphere intersection for inside the atmosphere.
float rayIntersectSphereNearest(vec3 ro, vec3 rd, float rad)
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

// Raymarch to compute the scattering integral.
vec3 raymarchScattering(float atmIndex, vec3 pos, vec3 rayDir, vec3 sunDir,
                        float tMax, ScatteringParams params, float groundRadiusMM,
                        float atmoRadiusMM);
void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  vec2 size = vec2(imageSize(skyviewImages).xy);
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) / size;

  const int atmosphereIndex = u_atmosphereIndices[invoke.z];
  const AtmosphereParams params = u_params[atmosphereIndex];

  float groundRadiusMM = params.planetAlbedoRadius.w;
  float atmosphereRadiusMM = params.sunDirAtmRadius.w;

  vec3 sunDir = normalize(-params.sunDirAtmRadius.xyz);
  vec3 viewPos = vec3(params.viewPos.xyz);

  float azimuthAngle = 2.0 * PI * (uv.x - 0.5);
  float coord = (uv.y < 0.5) ? 1.0 - 2.0 * uv.y : 2.0 * uv.y - 1.0;
  float adjV = (uv.y < 0.5) ? -(coord * coord) : coord * coord;

  float height = length(viewPos);
  vec3 up = viewPos / height;
  float horizonAngle = acos(clamp(sqrt(height * height - groundRadiusMM * groundRadiusMM)
                       / height, 0.0, 1.0)) - 0.5 * PI;
  float altitudeAngle = adjV * 0.5 * PI - horizonAngle;
  vec3 rayDir = normalize(vec3(cos(altitudeAngle) * sin(azimuthAngle),
                               sin(altitudeAngle), -cos(altitudeAngle) * cos(azimuthAngle)));

  float sunAltitude = (0.5 * PI) - acos(dot(sunDir, up));
  vec3 newSunDir = normalize(vec3(0.0, sin(sunAltitude), -cos(sunAltitude)));

  bool outOfAtmo = height >= atmosphereRadiusMM;
  float atmoEdge = rayIntersectSphereNearest(viewPos, rayDir, atmosphereRadiusMM);
  float planetRadiusOffset = (atmosphereRadiusMM - groundRadiusMM) / 100.0;
  float offset = 1e-4;
  viewPos = (outOfAtmo && atmoEdge >= 0.0) ? viewPos + (atmoEdge + offset) * rayDir : viewPos;

  float atmoDist = rayIntersectSphereNearest(viewPos, rayDir, atmosphereRadiusMM);
  float groundDist = rayIntersectSphereNearest(viewPos, rayDir, groundRadiusMM);
  float maxDist = (groundDist < 0.0) ? atmoDist : groundDist;

  vec3 lum = raymarchScattering(float(atmosphereIndex), viewPos, rayDir, newSunDir,
                                maxDist, params.sParams, groundRadiusMM,
                                atmosphereRadiusMM);
  lum *= params.lightColourIntensity.rgb * params.lightColourIntensity.w;
  lum = (atmoEdge < 0.0 && outOfAtmo) ? vec3(0.0) : lum;
  imageStore(skyviewImages, ivec3(invoke.xy, atmosphereIndex), vec4(lum, 1.0));
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

  return rayleighScattering + rayleighAbsorption + mieScattering + mieAbsorption + ozoneAbsorption;
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

vec3 raymarchScattering(float atmIndex, vec3 pos, vec3 rayDir, vec3 sunDir,
                        float tMax, ScatteringParams params, float groundRadiusMM,
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
    vec3 mieScattering = computeMieScattering(newPos, params, groundRadiusMM);

    vec3 sampleTransmittance = exp(-dt * extinction);

    vec3 sunTransmittance = getValFromLUT(atmIndex, transLUTs, newPos, sunDir,
                                          groundRadiusMM, atmoRadiusMM);

    vec3 psiMS = getValFromLUT(atmIndex, multiScatLUTs, newPos, sunDir,
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
