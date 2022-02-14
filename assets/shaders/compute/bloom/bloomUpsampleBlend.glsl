#type compute
#version 460 core
/*
 * A bloom upsampling and blending compute shader.
*/

layout(local_size_x = 8, local_size_y = 8) in;

// The previous mip in the upsampling mip chain.
layout(rgba16f, binding = 0) readonly uniform image2D previousUpscaleImage;

// The equivalent downsample mip to the next mip in the upsample chain, it will
// be filtered and added to the previous mip of the upsampling chain.
layout(rgba16f, binding = 1) readonly uniform image2D currentDownscaleImage;

// The next mip in the upsampling mip chain.
layout(rgba16f, binding = 2) restrict writeonly uniform image2D nextUpscaleImage;

layout(std140, binding = 3) readonly buffer PrefilterParams
{
  vec4 u_filterParams; // Threshold (x), threshold - knee (y), 2.0 * knee (z) and 0.25 / knee (w).
  vec2 u_upsampleRadius; // Upsampling filter radius (x).
};

// Interpolating sampler.
vec3 samplePrevious(ivec2 centerCoords, ivec2 offsetCoords, float radius);

// The upsampling tent filter.
vec3 upsampleBoxTentPrevious(ivec2 coords, float radius);

void main()
{
  ivec2 destCoords = ivec2(gl_GlobalInvocationID.xy);
  ivec2 sourceCoords = destCoords / 2;

  vec3 previousColour = upsampleBoxTentPrevious(sourceCoords, u_upsampleRadius.x);
  vec3 filteredColour = imageLoad(currentDownscaleImage, destCoords).rgb;
  vec3 blendedColour = filteredColour + previousColour;
  imageStore(nextUpscaleImage, destCoords, vec4(blendedColour, 1.0));
}

// Interpolating sampler.
vec3 samplePrevious(ivec2 centerCoords, ivec2 offsetCoords, float radius)
{
  ivec2 right0 = centerCoords + int(floor(radius)) * ivec2(offsetCoords.x, 0);
  ivec2 topRight0 = centerCoords + int(floor(radius)) * offsetCoords;
  ivec2 top0 = centerCoords + int(floor(radius)) * ivec2(0, offsetCoords.y);

  ivec2 right1 = centerCoords + int(ceil(radius)) * ivec2(offsetCoords.x, 0);
  ivec2 topRight1 = centerCoords + int(ceil(radius)) * offsetCoords;
  ivec2 top1 = centerCoords + int(ceil(radius)) * ivec2(0, offsetCoords.y);

  ivec2 sampleCoords0 = centerCoords + int(floor(radius)) * offsetCoords;
  vec3 sample0 = imageLoad(previousUpscaleImage, sampleCoords0).rgb;

  ivec2 sampleCoords1 = centerCoords + int(ceil(radius)) * offsetCoords;
  vec3 sample1 = imageLoad(previousUpscaleImage, sampleCoords1).rgb;

  return mix(sample0, sample1, sign(fract(radius)));
}

/*
  A  B  C
  D  E  F
  G  H  I
*/
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
vec3 upsampleBoxTentPrevious(ivec2 coords, float radius)
{
  vec3 A = samplePrevious(coords, ivec2(-1, 1), radius);
  vec3 B = samplePrevious(coords, ivec2(0, 1), radius) * 2.0;
  vec3 C = samplePrevious(coords, ivec2(1, 1), radius);
  vec3 D = samplePrevious(coords, ivec2(-1, 0), radius) * 2.0;
  vec3 E = samplePrevious(coords, ivec2(0, 0), radius) * 4.0;
  vec3 F = samplePrevious(coords, ivec2(1, 0), radius) * 2.0;
  vec3 G = samplePrevious(coords, ivec2(-1, -1), radius);
  vec3 H = samplePrevious(coords, ivec2(0, -1), radius) * 2.0;
  vec3 I = samplePrevious(coords, ivec2(1, -1), radius);

  return (A + B + C + D + E + F + G + H + I) * 0.0625; // * 1/16
}
