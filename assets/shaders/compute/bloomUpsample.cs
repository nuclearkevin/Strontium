#version 440

/*
 * A bloom upsampling compute shader.
*/

layout(local_size_x = 32, local_size_y = 32) in;

// The equivalent downsample mip to the next mip in the upsample chain, it will
// be filtered and added to the previous mip of the upsampling chain.
layout(rgba16f, binding = 0) readonly uniform image2D blurImage;

// The next mip in the upsampling mip chain.
layout(rgba16f, binding = 1) writeonly uniform image2D nextImage;

// The upsampling tend filter.
vec3 upsampleBoxTent(ivec2 coords);

void main()
{
  ivec2 sourceCoords = ivec2(gl_GlobalInvocationID.xy);

  vec3 filteredColour = upsampleBoxTent(sourceCoords);
  imageStore(nextImage, sourceCoords, vec4(filteredColour, 1.0));
}

/*
  A  B  C
  D  E  F
  G  H  I
*/
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
vec3 upsampleBoxTent(ivec2 coords)
{
  vec3 A = imageLoad(blurImage, coords + ivec2(-1, 1)).rgb;
  vec3 B = imageLoad(blurImage, coords + ivec2(0, 1)).rgb * 2.0;
  vec3 C = imageLoad(blurImage, coords + ivec2(1, 1)).rgb;
  vec3 D = imageLoad(blurImage, coords + ivec2(-1, 0)).rgb * 2.0;
  vec3 E = imageLoad(blurImage, coords).rgb * 4.0;
  vec3 F = imageLoad(blurImage, coords + ivec2(1, 0)).rgb * 2.0;
  vec3 G = imageLoad(blurImage, coords + ivec2(-1, -1)).rgb;
  vec3 H = imageLoad(blurImage, coords + ivec2(0, -1)).rgb * 2.0;
  vec3 I = imageLoad(blurImage, coords + ivec2(1, -1)).rgb;

  return (A + B + C + D + E + F + G + H + I) * 0.0625; // * 1/16
}
