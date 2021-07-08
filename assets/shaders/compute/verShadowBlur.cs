#version 440

layout(local_size_x = 32, local_size_y = 32) in;

// The input image to blur.
layout(rg32f, binding = 0) uniform readonly image2D inputImage;
// The output blurred image.
layout(rg32f, binding = 1) writeonly uniform image2D outputImage;

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  float weights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
  vec2 result = imageLoad(inputImage, invoke).rg * weights[0];
  for (uint i = 0; i < 5; i++)
  {
    result += imageLoad(inputImage, invoke + ivec2(0, i)).rg * weights[i];
    result += imageLoad(inputImage, invoke - ivec2(0, i)).rg * weights[i];
  }

  result = clamp(result, vec2(0.0), vec2(1.0));
  imageStore(outputImage, invoke, vec4(result, 0.0, 1.0));
}
