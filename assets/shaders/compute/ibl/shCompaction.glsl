#type compute
#version 460 core
/*
 * A compute shader to compact the SH coefficients and normalize them.
 */

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

#define MAX_SH_TOTAL 512
#define MAX_NUM_ATMOSPHERES 8
#define MAX_NUM_DYN_SKY_IBL 8
#define PI 3.141592654

// A packed struct for SH coefficients.
struct SHCoefficients
{
  vec4 L00; // Zonal
  vec4 L11;
  vec4 L10; // Zonal
  vec4 L1_1;
  vec4 L21;
  vec4 L2_1;
  vec4 L2_2;
  vec4 L20; // Zonal
  vec4 L22;
  vec4 weightSum; // Sum of weights (x). y, z and w are empty.
};

layout(std140, binding = 1) uniform SHParams
{
  ivec4 u_shParams; // Maximum number of SH coefficients (x), number of SH coefficients after the previous iteration (y) and the batch size (z). w is unused.
};

layout(std430, binding = 0) readonly buffer Indices
{
  int atmosphereIndices[MAX_NUM_ATMOSPHERES];
  int iblIndices[MAX_NUM_DYN_SKY_IBL];
};

layout(std430, binding = 1) readonly buffer UncompactedHarmonics
{
  SHCoefficients uncompacted[];
};

layout(std430, binding = 2) writeonly buffer CompactedHarmonics
{
  SHCoefficients compacted[];
};

SHCoefficients normalizeSH(SHCoefficients sh);

void main()
{
  const uint batchSize = uint(u_shParams.z);

  const ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  const uint slice = uint(invoke.x);
  const int iblIndex = iblIndices[slice];
  
  // Handle the case when we have too many threads. Extra threads can return.
  if (slice >= batchSize)
    return;

  // Offset into the buffer.
  const uint offset = iblIndex * MAX_SH_TOTAL; 
  // Fetch the uncompacted SH, normalize it, and write it to the compacted buffer.
  compacted[iblIndex] = normalizeSH(uncompacted[offset]);
}

// https://github.com/swr06/VoxelPathTracer/blob/Project-Main/Core/Shaders/Utility/sh.glsl
SHCoefficients normalizeSH(SHCoefficients sh)
{
  // l, m = 0, 0
  sh.L00.rgb *= 4.0 * PI / sh.weightSum.x;

  // l, m = 1, -1
  sh.L1_1.rgb *= 4.0 * PI / sh.weightSum.x;
  // l, m = 1, 0
  sh.L10.rgb *= 4.0 * PI / sh.weightSum.x;
  // l, m = 1, 1
  sh.L11.rgb *= 4.0 * PI / sh.weightSum.x;

  // l, m = 2, -2
  sh.L2_2.rgb *= 4.0 * PI / sh.weightSum.x;
  // l, m = 2, -1
  sh.L2_1.rgb *= 4.0 * PI / sh.weightSum.x;
  // l, m = 2, 0
  sh.L20.rgb *= 4.0 * PI / sh.weightSum.x;
  // l, m = 2, 1
  sh.L21.rgb *= 4.0 * PI / sh.weightSum.x;
  // l, m = 2, 2
  sh.L22.rgb *= 4.0 * PI / sh.weightSum.x;

  sh.weightSum.x *= 4.0 * PI / sh.weightSum.x;

  return sh;
}