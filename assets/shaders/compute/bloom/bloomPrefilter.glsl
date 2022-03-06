#type compute
#version 460 core
/*
 * A bloom prefiltering compute shader. Based off the presentation by
 * Jorge Jimenez at Advances in Real-Time Rendering:
 * http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
*/

layout(local_size_x = 8, local_size_y = 8) in;

// The input image from the lighting pass FBO.
layout(rgba16f, binding = 0) readonly uniform image2D lightingPassImage;

// The prefiltered output image, ready for downsampling.
layout(rgba16f, binding = 1) restrict writeonly uniform image2D prefilteredImage;

layout(std140, binding = 0) uniform PrefilterParams
{
  vec4 u_filterParams; // Threshold (x), threshold - knee (y), 2.0 * knee (z) and 0.25 / knee (w).
  vec2 u_upsampleRadius; // Upsampling filter radius (x) and the current mip-chain LOD (y). z and w are unused.
};

// The threshold curve.
vec3 quadraticThreshold(vec3 colour, float threshold, vec3 curve);

// The downsampling filter.
vec3 downsampleBox13Tap(ivec2 coords);

void main()
{
  ivec2 destCoords = ivec2(gl_GlobalInvocationID.xy);
  ivec2 sourceCoords = 2 * destCoords;

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

float computeWeight(vec3 colour)
{
  float luma = dot(vec3(0.2126, 0.7152, 0.0722), colour);
  return 1.0 / (1.0 + luma);
}

vec3 bilinearFetch(ivec2 coords)
{
  vec3 current = imageLoad(lightingPassImage, coords).rgb;
  vec3 right = imageLoad(lightingPassImage, coords + ivec2(1, 0)).rgb;
  vec3 top = imageLoad(lightingPassImage, coords + ivec2(0, 1)).rgb;
  vec3 topRight = imageLoad(lightingPassImage, coords + ivec2(1, 1)).rgb;

  return 0.25 * (current + right + top + topRight);
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
  float wA = computeWeight(A);
  A *= wA;
  vec3 B = bilinearFetch(coords + ivec2(0, 2));
  float wB = computeWeight(B);
  B *= wB;
  vec3 C = bilinearFetch(coords + ivec2(2, 2));
  float wC = computeWeight(C);
  C *= wC;
  vec3 D = bilinearFetch(coords + ivec2(-2, 0));
  float wD = computeWeight(D);
  D *= wD;
  vec3 E = bilinearFetch(coords + ivec2(0, 0));
  float wE = computeWeight(E);
  E *= wE;
  vec3 F = bilinearFetch(coords + ivec2(2, 0));
  float wF = computeWeight(F);
  F *= wF;
  vec3 G = bilinearFetch(coords + ivec2(-2, -2));
  float wG = computeWeight(G);
  G *= wG;
  vec3 H = bilinearFetch(coords + ivec2(0, -2));
  float wH = computeWeight(H);
  H *= wH;
  vec3 I = bilinearFetch(coords + ivec2(2, -2));
  float wI = computeWeight(I);
  I *= wI;

  vec3 J = bilinearFetch(coords + ivec2(-1, 1));
  float wJ = computeWeight(J);
  J *= wJ;
  vec3 K = bilinearFetch(coords + ivec2(1, 1));
  float wK = computeWeight(K);
  K *= wK;
  vec3 L = bilinearFetch(coords + ivec2(-1, -1));
  float wL = computeWeight(L);
  L *= wL;
  vec3 M = bilinearFetch(coords + ivec2(1, -1));
  float wM = computeWeight(M);
  M *= wM;

  vec2 weights = 0.25 * vec2(0.5, 0.125);
  vec3 result = (J + K + L + M) * weights.x / (wJ + wK + wL + wM);
  result += (A + B + D + E) * weights.y / (wA + wB + wD + wE);;
  result += (B + C + E + F) * weights.y / (wB + wC + wE + wF);;
  result += (D + E + G + H) * weights.y / (wD + wE + wG + wH);;
  result += (E + F + H + I) * weights.y / (wE + wF + wH + wI);;

  return result;
}
