#type compute
#version 460 core
/*
 * A compute shader for clearing a 3D texture.
 */

layout(rgba16f, binding = 0) restrict writeonly uniform image3D outImage;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
  imageStore(outImage, ivec3(gl_GlobalInvocationID.xyz), vec4(0.0.xxx, 1.0));
}
