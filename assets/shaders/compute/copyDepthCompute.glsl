#type compute
#version 460 core
/*
 * A compute shader to copy depth values.
*/

layout(local_size_x = 8, local_size_y = 8) in;

// The output depth mip.
layout(r32f, binding = 0) restrict writeonly uniform image2D zOut;

// The input depth buffer.
layout(binding = 0) uniform sampler2D zIn;

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 gBufferCoords = (vec2(invoke) + 0.5.xx) / vec2(textureSize(zIn, 0).xy);

  float d = texture(zIn, gBufferCoords).r;

  imageStore(zOut, invoke, vec4(d));
}
