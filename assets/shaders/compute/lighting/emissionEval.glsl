#type compute
#version 460 core
/*
 * A compute shader for adding object emission to the lighting buffer.
 */

layout(local_size_x = 8, local_size_y = 8) in;

// GBuffer.
layout(binding = 4) uniform sampler2D gEmission;

 // The lighting buffer.
layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  ivec2 gBufferSize = ivec2(textureSize(gEmission, 0).xy);

  // Quit early for threads that aren't in bounds of the screen.
  if (any(greaterThanEqual(invoke, gBufferSize)))
    return;

  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;
  totalRadiance += texelFetch(gEmission, invoke, 0).rgb;
  imageStore(lightingBuffer, invoke, vec4(totalRadiance, 1.0));
}