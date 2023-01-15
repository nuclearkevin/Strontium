#type compute
#version 460 core
/*
 * A compute shader to compute the diffuse lighting integral through
 * an SH projection. Modified to sample the sky view LUT of
 * Hillaire2020.
 */

#define MAX_NUM_ATMOSPHERES 8
#define MAX_NUM_DYN_SKY_IBL 8
#define PI 3.141592654

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// A packed struct for SH coefficients.
struct SHCoefficients
{
  vec4 L00;
  vec4 L11;
  vec4 L10;
  vec4 L1_1;
  vec4 L21;
  vec4 L2_1;
  vec4 L2_2;
  vec4 L20;
  vec4 L22;
  vec4 weightSum; // Sum of weights (x). y, z and w are empty.
};

//------------------------------------------------------------------------------
// Params from the sky atmosphere pass to read the sky-view LUT.
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
//------------------------------------------------------------------------------

// The skyview LUT to project to SH.
layout(binding = 0) uniform sampler2DArray skyViewLUT;

layout(std140, binding = 0) uniform HillaireParams
{
  AtmosphereParams u_atmoParams[MAX_NUM_ATMOSPHERES];
};

layout(std430, binding = 0) readonly buffer Indices
{
  int atmosphereIndices[MAX_NUM_ATMOSPHERES];
  int iblIndices[MAX_NUM_DYN_SKY_IBL];
};

layout(std430, binding = 1) writeonly buffer Harmonics
{
  SHCoefficients globalCoefficients[];
};

const float Y00 = 0.28209479177387814347f; // 1 / (2*sqrt(pi))
const float Y11 = -0.48860251190291992159f; // sqrt(3 /(4pi))
const float Y10 = 0.48860251190291992159f;
const float Y1_1 = -0.48860251190291992159f;
const float Y21 = -1.09254843059207907054f; // 1 / (2*sqrt(pi))
const float Y2_1 = -1.09254843059207907054f;
const float Y2_2 = 1.09254843059207907054f;
const float Y20 = 0.31539156525252000603f; // 1/4 * sqrt(5/pi)
const float Y22 = 0.54627421529603953527f; // 1/4 * sqrt(15/pi)

const float cosineLobeBandFactors[] = 
{
    PI, 2.0 * PI / 3.0, 2.0 * PI / 3.0, 2.0 * PI / 3.0,
    PI / 4.0, PI / 4.0, PI / 4.0, PI / 4.0, PI / 4.0
};

void projectOnSH(inout SHCoefficients sh, vec3 direction, vec3 value, float weight);
void sumSH(inout SHCoefficients accum, in SHCoefficients target);

shared SHCoefficients shCoefficients[64];

void main()
{
  vec2 size = vec2(textureSize(skyViewLUT, 0).xy);
  const uint lInvoke = uint(gl_LocalInvocationIndex);
  const ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) / size;

  const int slice = int(invoke.z);
  const int atmoIndex = atmosphereIndices[slice];
  const int iblIndex = iblIndices[slice];
  const AtmosphereParams params = u_atmoParams[atmoIndex];
  
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

  // Approximate differential solid angle of the texel.
  float weight = 2.0 * PI / (6.0 * size.x * size.y);

  // Project to SH and store in TLS.
  projectOnSH(shCoefficients[lInvoke], rayDir, texture(skyViewLUT, vec3(uv, float(atmoIndex))).rgb, weight);
  barrier();

  // Parallel reduction to sum the results in TLS.
  // 64 to 32
  if (lInvoke < 32)
    sumSH(shCoefficients[lInvoke], shCoefficients[lInvoke + 32]);
  barrier();

  // 32 to 16
  if (lInvoke < 16)
    sumSH(shCoefficients[lInvoke], shCoefficients[lInvoke + 16]);
  barrier();

  // 16 to 8.
  if (lInvoke < 8)
    sumSH(shCoefficients[lInvoke], shCoefficients[lInvoke + 8]);
  barrier();

  // 8 to 4.
  if (lInvoke < 4)
    sumSH(shCoefficients[lInvoke], shCoefficients[lInvoke + 4]);
  barrier();

  // 4 to 2.
  if (lInvoke < 2)
    sumSH(shCoefficients[lInvoke], shCoefficients[lInvoke + 2]);
  barrier();

   // 2 to 1.
  if (lInvoke < 1)
    sumSH(shCoefficients[lInvoke], shCoefficients[lInvoke + 1]);
  barrier();

  if (lInvoke > 0)
    return;

  // Write the resulting SH coefficients to a global buffer for further reduction.
  const ivec2 totalNumGroups = ivec2(gl_NumWorkGroups.xy); 
  const ivec2 currentGroup = ivec2(gl_WorkGroupID.xy); 
  const uint offset = iblIndex * (totalNumGroups.x * totalNumGroups.y) + (totalNumGroups.x * currentGroup.y + currentGroup.x); // Offset into the buffer
  globalCoefficients[offset] = shCoefficients[0];
}

// https://github.com/swr06/VoxelPathTracer/blob/Project-Main/Core/Shaders/Utility/sh.glsl
void projectOnSH(inout SHCoefficients sh, vec3 direction, vec3 value, float weight)
{
  // l, m = 0, 0
  sh.L00.rgb = value * Y00 * cosineLobeBandFactors[0] * weight;

  // l, m = 1, -1
  sh.L1_1.rgb = value * Y1_1 * cosineLobeBandFactors[1] * direction.y * weight;
  // l, m = 1, 0
  sh.L10.rgb = value * Y10 * cosineLobeBandFactors[2] * direction.z * weight;
  // l, m = 1, 1
  sh.L11.rgb = value * Y11 * cosineLobeBandFactors[3] * direction.x * weight;

  // l, m = 2, -2
  sh.L2_2.rgb = value * Y2_2 * cosineLobeBandFactors[4] * (direction.x * direction.y) * weight;
  // l, m = 2, -1
  sh.L2_1.rgb = value * Y2_1 * cosineLobeBandFactors[5] * (direction.y * direction.z) * weight;
  // l, m = 2, 0
  sh.L20.rgb = value * Y20 * cosineLobeBandFactors[7] * (3.0f * direction.z * direction.z - 1.0f) * weight;
  // l, m = 2, 1
  sh.L21.rgb = value * Y21 * cosineLobeBandFactors[6] * (direction.x * direction.z) * weight;
  // l, m = 2, 2
  sh.L22.rgb = value * Y22 * cosineLobeBandFactors[8] * (direction.x * direction.x - direction.y * direction.y) * weight;

  sh.weightSum.x = weight;
}

// https://github.com/swr06/VoxelPathTracer/blob/Project-Main/Core/Shaders/Utility/sh.glsl
void sumSH(inout SHCoefficients accum, in SHCoefficients target)
{
  // l, m = 0, 0
  accum.L00.rgb += target.L00.rgb;

  // l, m = 1, -1
  accum.L1_1.rgb += target.L1_1.rgb;
  // l, m = 1, 0
  accum.L10.rgb += target.L10.rgb;
  // l, m = 1, 1
  accum.L11.rgb += target.L11.rgb;

  // l, m = 2, -2
  accum.L2_2.rgb += target.L2_2.rgb;
  // l, m = 2, -1
  accum.L2_1.rgb += target.L2_1.rgb;
  // l, m = 2, 0
  accum.L20.rgb += target.L20.rgb;
  // l, m = 2, 1
  accum.L21.rgb += target.L21.rgb;
  // l, m = 2, 2
  accum.L22.rgb += target.L22.rgb;

  accum.weightSum.x += target.weightSum.x;
}