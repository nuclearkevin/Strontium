#type compute
#version 460 core
/*
 * Compute shader to zero the RGB elements of a 2D texture.
*/

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) restrict writeonly uniform image2D outImage;

void main()
{
  imageStore(outImage, ivec2(gl_GlobalInvocationID.xy), vec4(0.0.xxx, 1.0));
}
