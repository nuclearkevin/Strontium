#type compute
#version 460 core
/*
 * Compute shader to generate the transmittance lookup texture as described in
 * [Hillaire2020].
 * https://sebh.github.io/publications/egsr2020.pdf
 * https://github.com/sebh/UnrealEngineSkyAtmosphere
 * https://www.shadertoy.com/view/slSXRW
*/

#define MAX_NUM_ATMOSPHERES 8
#define TRANSMITTANCE_STEPS 40

// x and y are the current texels in the lut [0, 256] x [0, 64]. z is the
// current atmosphere [0, 8].
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

layout(rgba16f, binding = 0) restrict writeonly uniform image2DArray transImages;

layout(std140, binding = 0) uniform HillaireParams
{
  AtmosphereParams u_params[MAX_NUM_ATMOSPHERES];
};

layout(std430, binding = 1) readonly buffer HillaireIndices
{
  int atmosphereIndices[MAX_NUM_ATMOSPHERES];
};

// Helper functions.
float safeACos(float x)
{
  return acos(clamp(x, -1.0, 1.0));
}

// Compute the transmittance.
vec3 getSunTransmittance(vec3 pos, vec3 sunDir, ScatteringParams params,
                         float groundRadiusMM, float atmoRadiusMM);

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  vec2 size = vec2(imageSize(transImages).xy);
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) / size;

  const int atmosphereIndex = atmosphereIndices[invoke.z];
  const AtmosphereParams params = u_params[atmosphereIndex];

  float groundRadiusMM = params.planetAlbedoRadius.w;
  float atmosphereRadiusMM = params.sunDirAtmRadius.w;

  float sunCosTheta = 2.0 * uv.x - 1.0;
  float sunTheta = safeACos(sunCosTheta);
  float height = mix(groundRadiusMM, atmosphereRadiusMM, uv.y);

  vec3 pos = vec3(0.0, height, 0.0);
  vec3 sunDir = normalize(vec3(0.0, sunCosTheta, -sin(sunTheta)));

  vec3 transmittance = getSunTransmittance(pos, sunDir, params.sParams, groundRadiusMM, atmosphereRadiusMM);

  imageStore(transImages, ivec3(invoke.xy, atmosphereIndex),
             vec4(max(transmittance, vec3(0.0)), 1.0));
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

vec3 getSunTransmittance(vec3 pos, vec3 sunDir, ScatteringParams params,
                         float groundRadiusMM, float atmoRadiusMM)
{
  if (rayIntersectSphere(pos, sunDir, groundRadiusMM) > 0.0)
    return vec3(0.0);

  float atmoDist = rayIntersectSphere(pos, sunDir, atmoRadiusMM);

  float t = 0.0;
  vec3 transmittance = vec3(1.0);
  for (uint i = 0; i < TRANSMITTANCE_STEPS; i++)
  {
    float newT = ((float(i) + 0.3) / float(TRANSMITTANCE_STEPS)) * atmoDist;
    float dt = newT - t;
    t = newT;

    vec3 newPos = pos + t * sunDir;
    vec3 extinction = computeExtinction(newPos, params, groundRadiusMM);

    transmittance *= exp(-dt * extinction);
  }

  return transmittance;
}
