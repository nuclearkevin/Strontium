#type compute
#version 460 core
/*
 * A bloom upsampling and blending compute shader.
*/

layout(local_size_x = 8, local_size_y = 8) in;

// The previous mip in the upsampling mip chain. It will
// be filtered and added to the previous mip of the upsampling chain.
layout(binding = 0) uniform sampler2D previousUpscale;

// The equivalent downsample mip to the next mip in the upsample chain, it will
// be filtered and added to the previous mip of the upsampling chain.
layout(rgba16f, binding = 0) readonly uniform image2D currentDownscaleImage;

// The next mip in the upsampling mip chain.
layout(rgba16f, binding = 1) restrict writeonly uniform image2D currentUpscale;

layout(std140, binding = 0) uniform PrefilterParams
{
  vec4 u_filterParams; // Threshold (x), threshold - knee (y), 2.0 * knee (z) and 0.25 / knee (w).
  vec2 u_upsampleRadius; // Upsampling filter radius (x) and the current mip-chain LOD (y). z and w are unused.
};

// The upsampling tent filter.
vec3 upsampleBoxTentPrevious(sampler2D tex, vec2 uvs, float lod, vec2 texel, float radius);

void main()
{
  ivec2 destCoords = ivec2(gl_GlobalInvocationID.xy);
  ivec2 sourceCoords = destCoords / 2;
  float lod = u_upsampleRadius.y;
  vec2 texel = 1.0.xx / vec2(textureSize(previousUpscale, int(lod)));
  vec2 sourceUVs = (vec2(sourceCoords) + 0.5.xx) * texel;

  vec3 previousColour = upsampleBoxTentPrevious(previousUpscale, sourceUVs, lod,
                                                texel, u_upsampleRadius.x);
  vec3 filteredColour = imageLoad(currentDownscaleImage, destCoords).rgb;
  imageStore(currentUpscale, destCoords, vec4(filteredColour + previousColour, 1.0));
}

/*
  A  B  C
  D  E  F
  G  H  I
*/
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
vec3 upsampleBoxTentPrevious(sampler2D tex, vec2 uvs, float lod, vec2 texel, float radius)
{
  vec3 A = textureLod(tex, uvs + texel * radius * vec2(-1.0, 1.0), lod).rgb;
  vec3 B = textureLod(tex, uvs + texel * radius * vec2(0.0, 1.0), lod).rgb * 2.0;
  vec3 C = textureLod(tex, uvs + texel * radius * vec2(1.0, 1.0), lod).rgb;
  vec3 D = textureLod(tex, uvs + texel * radius * vec2(-1.0, 0.0), lod).rgb * 2.0;
  vec3 E = textureLod(tex, uvs + texel * radius * vec2(0.0, 0.0), lod).rgb * 4.0;
  vec3 F = textureLod(tex, uvs + texel * radius * vec2(1.0, 0.0), lod).rgb * 2.0;
  vec3 G = textureLod(tex, uvs + texel * radius * vec2(-1.0, -1.0), lod).rgb;
  vec3 H = textureLod(tex, uvs + texel * radius * vec2(0.0, -1.0), lod).rgb * 2.0;
  vec3 I = textureLod(tex, uvs + texel * radius * vec2(1.0, -1.0), lod).rgb;

  return (A + B + C + D + E + F + G + H + I) * 0.0625; // * 1/16
}
