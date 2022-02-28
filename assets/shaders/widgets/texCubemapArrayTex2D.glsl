#type compute
#version 460 core
/*
 * Compute shader to convert a cubemap array to an equirectangular map.
*/

#define PI 3.141592654

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) restrict writeonly uniform image2D outImage;

layout(binding = 0) uniform samplerCubeArray cubemap;

layout(location = 0) uniform float u_lod = 0.0;
layout(location = 1) uniform float u_slice = 0.0;

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);

  vec2 uv = (vec2(invoke) + 0.5.xx) / vec2(imageSize(outImage).xy);
  float phi = PI * 2.0 * uv.x;
  float theta = PI * (-uv.y + 0.5);

  float cosTheta = cos(theta);
  vec3 dir = normalize(vec3(cos(phi) * cosTheta, sin(theta), sin(phi) * cosTheta));
  vec3 colour = textureLod(cubemap, vec4(dir, u_slice), u_lod).rgb;

  imageStore(outImage, invoke, vec4(colour, 1.0));
}
