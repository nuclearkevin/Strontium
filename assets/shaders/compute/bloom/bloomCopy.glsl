#type compute
#version 460 core
/*
 * A compute shader to copy an image from one mip chain to another.
*/

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) readonly uniform image2D downsample;

layout(rgba16f, binding = 1) restrict writeonly uniform image2D upsample;

void main()
{
  ivec2 sourceCoords = ivec2(gl_GlobalInvocationID.xy);

  vec3 colour = imageLoad(downsample, sourceCoords).rgb;

  imageStore(upsample, sourceCoords, vec4(colour, 1.0));
}
