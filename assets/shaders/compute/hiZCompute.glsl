#type compute
#version 460 core
/*
 * A compute shader to downsample the depth buffer into a hierarchical z buffer.
*/

layout(local_size_x = 8, local_size_y = 8) in;

// The output depth mip.
layout(r32f, binding = 0) restrict writeonly uniform image2D zOut;

// The input depth mip.
layout(r32f, binding = 1) readonly uniform image2D zIn;

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);

  // Previous mip coords.
  ivec2 previousCoords = 2 * invoke;

  ivec2 mipSize = ivec2(imageSize(zIn).xy);

  // Grab the nearest 4 depth coords from the previous mip.
  float d0 = imageLoad(zIn, previousCoords + ivec2(0, 0)).r;
  float d1 = imageLoad(zIn, previousCoords + ivec2(1, 0)).r;
  float d2 = imageLoad(zIn, previousCoords + ivec2(0, 1)).r;
  float d3 = imageLoad(zIn, previousCoords + ivec2(1, 1)).r;

  // Find and store the local max depth.
  float maxD = max(max(d0, d1), max(d2, d3));
  imageStore(zOut, invoke, vec4(maxD));
}
