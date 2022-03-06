#type compute
#version 460 core
/*
 * Compute shader to extract a single slice of a 2D texture array.
*/

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) restrict writeonly uniform image2D outImage;

layout(binding = 0) uniform sampler2DArray arrayTexture;

layout(location = 0) uniform float u_lod = 0.0;
layout(location = 1) uniform float u_slice = 0.0;

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);

  vec2 outputSize = vec2(imageSize(outImage).xy);
  vec2 inputSize = vec2(textureSize(arrayTexture, 0).xy);
  vec2 ratio = inputSize / outputSize;

  vec2 uv = ratio * (vec2(invoke) + 0.5.xx) / inputSize;
  vec3 colour = textureLod(arrayTexture, vec3(uv, u_slice), u_lod).rgb;

  imageStore(outImage, invoke, vec4(colour, 1.0));
}
