#version 440

/*
 * A bloom upsampling and blending compute shader.
*/

layout(local_size_x = 32, local_size_y = 32) in;

// The previous mip in the upsampling mip chain.
layout(rgba16f, binding = 0) readonly uniform image2D previousUpscaleImage;

// The equivalent downsample mip to the next mip in the upsample chain, it will
// be filtered and added to the previous mip of the upsampling chain.
layout(rgba16f, binding = 1) readonly uniform image2D currentDownscaleImage;

// The next mip in the upsampling mip chain.
layout(rgba16f, binding = 2) writeonly uniform image2D nextUpscaleImage;

// The upsampling tent filter.
vec3 upsampleBoxTentCurrent(ivec2 coords);
vec3 upsampleBoxTentPrevious(ivec2 coords);

void main()
{
  ivec2 destCoords = ivec2(gl_GlobalInvocationID.xy);
  ivec2 sourceCoords = destCoords / 2;

  vec3 previousColour = upsampleBoxTentPrevious(sourceCoords);
  vec3 filteredColour = imageLoad(currentDownscaleImage, destCoords).rgb;
  vec3 blendedColour = filteredColour + previousColour;
  imageStore(nextUpscaleImage, destCoords, vec4(blendedColour, 1.0));
}

/*
  A  B  C
  D  E  F
  G  H  I
*/
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
vec3 upsampleBoxTentCurrent(ivec2 coords)
{
  const int radius = 1;
  vec3 A = imageLoad(currentDownscaleImage, coords + radius * ivec2(-1, 1)).rgb;
  vec3 B = imageLoad(currentDownscaleImage, coords + radius * ivec2(0, 1)).rgb * 2.0;
  vec3 C = imageLoad(currentDownscaleImage, coords + radius * ivec2(1, 1)).rgb;
  vec3 D = imageLoad(currentDownscaleImage, coords + radius * ivec2(-1, 0)).rgb * 2.0;
  vec3 E = imageLoad(currentDownscaleImage, coords).rgb * 4.0;
  vec3 F = imageLoad(currentDownscaleImage, coords + radius * ivec2(1, 0)).rgb * 2.0;
  vec3 G = imageLoad(currentDownscaleImage, coords + radius * ivec2(-1, -1)).rgb;
  vec3 H = imageLoad(currentDownscaleImage, coords + radius * ivec2(0, -1)).rgb * 2.0;
  vec3 I = imageLoad(currentDownscaleImage, coords + radius * ivec2(1, -1)).rgb;

  return (A + B + C + D + E + F + G + H + I) * 0.0625; // * 1/16
}

vec3 upsampleBoxTentPrevious(ivec2 coords)
{
  const int radius = 1;
  vec3 A = imageLoad(previousUpscaleImage, coords + radius * ivec2(-1, 1)).rgb;
  vec3 B = imageLoad(previousUpscaleImage, coords + radius * ivec2(0, 1)).rgb * 2.0;
  vec3 C = imageLoad(previousUpscaleImage, coords + radius * ivec2(1, 1)).rgb;
  vec3 D = imageLoad(previousUpscaleImage, coords + radius * ivec2(-1, 0)).rgb * 2.0;
  vec3 E = imageLoad(previousUpscaleImage, coords).rgb * 4.0;
  vec3 F = imageLoad(previousUpscaleImage, coords + radius * ivec2(1, 0)).rgb * 2.0;
  vec3 G = imageLoad(previousUpscaleImage, coords + radius * ivec2(-1, -1)).rgb;
  vec3 H = imageLoad(previousUpscaleImage, coords + radius * ivec2(0, -1)).rgb * 2.0;
  vec3 I = imageLoad(previousUpscaleImage, coords + radius * ivec2(1, -1)).rgb;

  return (A + B + C + D + E + F + G + H + I) * 0.0625; // * 1/16
}
