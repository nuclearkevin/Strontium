#version 440

/*
 * A bloom prefiltering compute shader.
*/

layout(local_size_x = 32, local_size_y = 32) in;

// The input image from the lighting pass FBO.
layout(rgba16f, binding = 0) readonly uniform image2D lightingPassImage;

// The prefiltered output image, ready for downsampling.
layout(rgba16f, binding = 1) writeonly uniform image2D prefilteredImage;

// The knee and threshold.
layout(std140, binding = 3) buffer prefilterParams
{
  vec4 u_filterParams; // Threshold (x), threshold - knee (y), 2.0 * knee (z) and 0.25 / knee (w).
  float u_upsampleRadius;
};

// The threshold curve.
vec3 quadraticThreshold(vec3 colour, float threshold, vec3 curve);

// The downsampling filter.
vec3 downsampleBox13Tap(ivec2 coords);

// Bilinear fetching for 4 texels.
vec3 bilinearFetch(ivec2 coords);

void main()
{
  ivec2 destCoords = ivec2(gl_GlobalInvocationID.xy);
  ivec2 sourceCoords = destCoords;

  vec3 curve = vec3(u_filterParams.y, u_filterParams.z, u_filterParams.w);

  vec3 colour = downsampleBox13Tap(sourceCoords);
  vec3 filteredColour = max(quadraticThreshold(colour, u_filterParams.x, curve), vec3(0.0));

  imageStore(prefilteredImage, destCoords, vec4(filteredColour, 1.0));
}

vec3 quadraticThreshold(vec3 colour, float threshold, vec3 curve)
{
  float br = max(max(colour.r, colour.g), colour.b);
  float rq = clamp(br - curve.x, 0.0, curve.y);
  rq = curve.z * rq * rq;
  vec3 outColour = colour * max(rq, br - threshold) / max(br, 0.00001);
  return outColour;
}

/*
  A   B   C
    J   K
  D   E   F
    L   M
  G   H   I
*/
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
vec3 downsampleBox13Tap(ivec2 coords)
{
  vec3 A = bilinearFetch(coords + ivec2(-2, 2));
  vec3 B = bilinearFetch(coords + ivec2(0, 2));
  vec3 C = bilinearFetch(coords + ivec2(2, 2));
  vec3 D = bilinearFetch(coords + ivec2(-2, 0));
  vec3 E = bilinearFetch(coords + ivec2(0, 0));
  vec3 F = bilinearFetch(coords + ivec2(2, 0));
  vec3 G = bilinearFetch(coords + ivec2(-2, -2));
  vec3 H = bilinearFetch(coords + ivec2(0, -2));
  vec3 I = bilinearFetch(coords + ivec2(2, -2));

  vec3 J = bilinearFetch(coords + ivec2(-1, 1));
  vec3 K = bilinearFetch(coords + ivec2(1, 1));
  vec3 L = bilinearFetch(coords + ivec2(-1, -1));
  vec3 M = bilinearFetch(coords + ivec2(1, -1));

  vec2 weights = 0.25 * vec2(0.5, 0.125);
  vec3 result = (J + K + L + M) * weights.x;
  result += (A + B + D + E) * weights.y;
  result += (B + C + E + F) * weights.y;
  result += (D + E + G + H) * weights.y;
  result += (E + F + H + I) * weights.y;

  return result;
}

vec3 bilinearFetch(ivec2 coords)
{
  vec3 current = imageLoad(lightingPassImage, coords).rgb;
  vec3 right = imageLoad(lightingPassImage, coords + ivec2(1, 0)).rgb;
  vec3 top = imageLoad(lightingPassImage, coords + ivec2(0, 1)).rgb;
  vec3 topRight = imageLoad(lightingPassImage, coords + ivec2(1, 1)).rgb;

  return 0.25 * (current + right + top + topRight);
}
