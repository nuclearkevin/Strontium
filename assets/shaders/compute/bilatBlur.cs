#version 440

/*
 * A quick and dirty colour-based bilateral blur. Based off of the Shadertoy
 * implementation here: https://www.shadertoy.com/view/4dfGDH
*/

#define SIGMA 10.0
#define BSIGMA 0.1
#define MSIZE 15

const float kernel[MSIZE] = float[MSIZE]
(
  0.031225216, 0.033322271, 0.035206333, 0.036826804, 0.038138565, 0.039104044,
  0.039695028, 0.039894000, 0.039695028, 0.039104044, 0.038138565, 0.036826804,
  0.035206333, 0.033322271, 0.031225216
);

layout(local_size_x = 32, local_size_y = 32) in;

layout(rgba16f, binding = 0) readonly uniform image2D inImage;
layout(rgba16f, binding = 1) writeonly uniform image2D outImage;

float normpdf(float x, float sigma)
{
	return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

float normpdf4(vec4 v, float sigma)
{
	return 0.39894 * exp(-0.5 * dot(v,v) / (sigma * sigma)) / sigma;
}

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);

  vec4 c = imageLoad(inImage, invoke + ivec2(0, 0)).rgba;

  const int kSize = (MSIZE - 1) / 2;
  vec4 result = vec4(0.0);

  vec4 cc;
  float factor;
  float bZ = 1.0 / normpdf(0.0, BSIGMA);
  float Z = 0.0;
  for (int i = -kSize; i <= kSize; ++i)
  {
    for (int j = -kSize; j <= kSize; ++j)
    {
      cc = imageLoad(inImage, invoke + ivec2(i, j)).rgba;
      factor = normpdf4(cc - c, BSIGMA) * bZ * kernel[kSize + j] * kernel[kSize + i];
      Z += factor;
      result += factor * cc;
    }
  }

  imageStore(outImage, invoke, vec4(result / Z));
}
