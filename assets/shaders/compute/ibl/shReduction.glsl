#type compute
#version 460 core
/*
 * A compute shader to perform a reduction and sum the results of an SH projection.
 */

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

#define MAX_SH_TOTAL 512
#define SH_BATCH_SIZE 8
#define MAX_NUM_ATMOSPHERES 8
#define MAX_NUM_DYN_SKY_IBL 8

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

layout(std140, binding = 1) uniform SHParams
{
  ivec4 u_shParams; // Maximum number of SH coefficients (x), number of SH coefficients after the previous iteration (y) and the batch size (z). w is unused.
};

layout(std430, binding = 0) readonly buffer Indices
{
  int atmosphereIndices[MAX_NUM_ATMOSPHERES];
  int iblIndices[MAX_NUM_DYN_SKY_IBL];
};

layout(std430, binding = 1) buffer Harmonics
{
  SHCoefficients globalCoefficients[];
};

void sumSH(inout SHCoefficients accum, in SHCoefficients target);

shared SHCoefficients shCoefficients[64];

void main()
{
  const uint lInvoke = uint(gl_LocalInvocationIndex);
  const ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  const ivec2 currentGroup = ivec2(gl_WorkGroupID.xy); 
  const int slice = int(invoke.z);
  const int iblIndex = iblIndices[slice];
  
  // Offset into the buffer.
  const uint offset = iblIndex * MAX_SH_TOTAL + SH_BATCH_SIZE * lInvoke;
  SHCoefficients sh0 = globalCoefficients[offset];
  sumSH(sh0, globalCoefficients[offset + 1]);
  sumSH(sh0, globalCoefficients[offset + 2]);
  sumSH(sh0, globalCoefficients[offset + 3]);
  sumSH(sh0, globalCoefficients[offset + 4]);
  sumSH(sh0, globalCoefficients[offset + 5]);
  sumSH(sh0, globalCoefficients[offset + 6]);
  sumSH(sh0, globalCoefficients[offset + 7]);

  shCoefficients[lInvoke] = sh0;
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

  globalCoefficients[iblIndex * MAX_SH_TOTAL] = shCoefficients[0];
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