#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of image-based lighting.
 */

#define MAX_NUM_ATMOSPHERES 8
#define PI 3.141592654
#define MAX_MIP 6.0

layout(local_size_x = 8, local_size_y = 8) in;

struct SurfaceProperties
{
  vec3 albedo;
  vec3 metallicF0;
  vec3 dielectricF0;
  float metalness;
  float roughness;
  float ao;
  float emission;

  vec3 position;
  vec3 normal;
  vec3 view;
};

// A packed struct for SH coefficients.
struct SHCoefficients
{
  vec4 L00;
  vec4 L11;
  vec4 L10;
  vec4 L1_1;
  vec4 L21;
  vec4 L2_1;
  vec4 L2_2;
  vec4 L20;
  vec4 L22;
  vec4 weightSum; // Sum of weights (x). y, z and w are empty.
};

//------------------------------------------------------------------------------
// Params from the sky atmosphere pass to read the sky-view LUT.
// Atmospheric scattering coefficients are expressed per unit km.
struct ScatteringParams
{
  vec4 rayleighScat; //  Rayleigh scattering base (x, y, z) and height falloff (w).
  vec4 rayleighAbs; //  Rayleigh absorption base (x, y, z) and height falloff (w).
  vec4 mieScat; //  Mie scattering base (x, y, z) and height falloff (w).
  vec4 mieAbs; //  Mie absorption base (x, y, z) and height falloff (w).
  vec4 ozoneAbs; //  Ozone absorption base (x, y, z) and scale (w).
};

// Radii are expressed in megameters (MM).
struct AtmosphereParams
{
  ScatteringParams sParams;
  vec4 planetAlbedoRadius; // Planet albedo (x, y, z) and radius.
  vec4 sunDirAtmRadius; // Sun direction (x, y, z) and atmosphere radius (w).
  vec4 lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  vec4 viewPos; // View position (x, y, z). w is unused.
};
//------------------------------------------------------------------------------

// GBuffer.
layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMatProp;
layout(binding = 4) uniform sampler2D gEmission;

// TODO: Spherical harmonics instead of a cubemap for irradiance.
layout(binding = 6) uniform samplerCubeArray radianceMap;
layout(binding = 7) uniform sampler2D brdfLookUp;
layout(binding = 8) uniform sampler2D hbaoTexture;

// The lighting buffer.
layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y). z and w are unused.
};

// Ambient lighting specific uniforms.
layout(std140, binding = 1) uniform IBLBlock
{
  vec4 u_iblParams; // IBL index (x), IBL intensity (y), use HBAO (z), atmosphere index (w).
};

layout(std140, binding = 2) uniform HillaireParams
{
  AtmosphereParams u_atmoParams[MAX_NUM_ATMOSPHERES];
};

layout(std430, binding = 0) readonly buffer CompactedHarmonics
{
  SHCoefficients compactedSH[];
};

const float Y00 = 0.28209479177387814347f; // 1 / (2*sqrt(pi))
const float Y11 = -0.48860251190291992159f; // sqrt(3 /(4pi))
const float Y10 = 0.48860251190291992159f;
const float Y1_1 = -0.48860251190291992159f;
const float Y21 = -1.09254843059207907054f; // 1 / (2*sqrt(pi))
const float Y2_1 = -1.09254843059207907054f;
const float Y2_2 = 1.09254843059207907054f;
const float Y20 = 0.31539156525252000603f; // 1/4 * sqrt(5/pi)
const float Y22 = 0.54627421529603953527f; // 1/4 * sqrt(15/pi)

// Decode the g-buffer.
SurfaceProperties decodeGBuffer(vec2 gBufferUVs, ivec2 gBufferTexel);

// Transform the normal vector to a sky-view LUT lookup vector.
vec3 normalToSky(vec3 normal, AtmosphereParams atmo);

// Evaluate the diffuse SH coefficients.
vec3 evaluateDiffSH(SHCoefficients sh, vec3 normal);

// Compute the static IBL contribution.
vec3 evaluateIBL(float iblIndex, SurfaceProperties gBuffer, AtmosphereParams atmo, float diffAO);

// Fetches and upsamples the horizon-based ambient occlusion.
float getHBAO(sampler2D gDepth, sampler2D hbaoTexture, vec2 gBufferCoords);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  ivec2 gBufferSize = ivec2(textureSize(gDepth, 0).xy);

  // Quit early for threads that aren't in bounds of the screen.
  if (any(greaterThanEqual(invoke, gBufferSize)))
    return;

  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(textureSize(gDepth, 0).xy);

  if (texelFetch(gDepth, invoke, 0).r >= (1.0 - 1e-6))
    return;

  SurfaceProperties gBuffer = decodeGBuffer(gBufferUVs, invoke);

  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;

  float ao = gBuffer.ao;
  AtmosphereParams atmo = u_atmoParams[uint(u_iblParams.w)];
  if (u_iblParams.z > 0.0)
    ao = min(getHBAO(gDepth, hbaoTexture, gBufferUVs), ao);

  totalRadiance += evaluateIBL(u_iblParams.x, gBuffer, atmo, ao) * u_iblParams.y;
  totalRadiance += texelFetch(gEmission, invoke, 0).rgb;

  imageStore(lightingBuffer, invoke, vec4(totalRadiance, 1.0));
}

// Decodes the worldspace position of the fragment from depth.
vec3 decodePosition(vec2 texCoords, sampler2D depthMap, mat4 invMVP, ivec2 texel)
{
  float depth = texelFetch(depthMap, texel, 0).r;
  vec3 clipCoords = 2.0 * vec3(texCoords, depth) - 1.0.xxx;
  vec4 temp = invMVP * vec4(clipCoords, 1.0);
  return temp.xyz / temp.w;
}
// Fast octahedron normal vector decoding.
// https://jcgt.org/published/0003/02/01/
vec2 signNotZero(vec2 v)
{
  return vec2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
}
vec3 decodeNormal(vec2 texCoords, sampler2D encodedNormals, ivec2 texel)
{
  vec2 e = texelFetch(encodedNormals, texel, 0).xy;
  vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
  if (v.z < 0)
    v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
  return normalize(v);
}

SurfaceProperties decodeGBuffer(vec2 gBufferUVs, ivec2 gBufferTexel)
{
  SurfaceProperties decoded;
  decoded.position = decodePosition(gBufferUVs, gDepth, u_invViewProjMatrix, gBufferTexel);
  decoded.view = normalize(u_camPosition - decoded.position);
  decoded.normal = decodeNormal(gBufferUVs, gNormal, gBufferTexel);

  vec4 mra = texelFetch(gMatProp, gBufferTexel, 0);
  decoded.metalness = mra.r;
  decoded.roughness = mra.g;
  decoded.ao = mra.b;
  decoded.emission = 0.0;

  // Remap material properties.
  vec4 albedoReflectance = texelFetch(gAlbedo, gBufferTexel, 0).rgba;
  decoded.albedo = albedoReflectance.rgb;
  decoded.dielectricF0 = 0.16 * albedoReflectance.aaa * albedoReflectance.aaa;
  decoded.metallicF0 = decoded.albedo * decoded.metalness;

  return decoded;
}

// A fast approximation of specular AO given the diffuse AO. [Lagarde14]
float computeSpecularAO(float nDotV, float ao, float roughness)
{
  return clamp(pow(nDotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao, 0.0, 1.0);
}

float safeACos(float x)
{
  return acos(clamp(x, -1.0, 1.0));
}

vec3 normalToSky(vec3 normal, AtmosphereParams atmo)
{
  float height = length(atmo.viewPos.xyz);
  vec3 up = atmo.viewPos.xyz / height;
  vec3 sunDir = normalize(atmo.sunDirAtmRadius.xyz);

  float horizonAngle = safeACos(sqrt(height * height - atmo.planetAlbedoRadius.w * atmo.planetAlbedoRadius.w) / height);
  float altitudeAngle = horizonAngle - acos(dot(normal, up));

  vec3 right = cross(sunDir, up);
  vec3 forward = cross(up, right);
  vec3 projectedDir = normalize(normal - up * (dot(normal, up)));
  float sinTheta = dot(projectedDir, right);
  float cosTheta = dot(projectedDir, forward);
  float azimuthAngle = atan(sinTheta, cosTheta) + PI;

  vec3 rayDir = normalize(vec3(cos(altitudeAngle) * sin(azimuthAngle),
                               sin(altitudeAngle), -cos(altitudeAngle) * cos(azimuthAngle)));

  return rayDir;
}

// Evaluate the diffuse SH coefficients.
vec3 evaluateDiffSH(SHCoefficients sh, vec3 normal)
{
  vec3 result = 0.0.xxx;

  result += sh.L00.rgb * Y00;

  result += sh.L1_1.rgb * Y1_1 * normal.y;
  result += sh.L10.rgb * Y10 * normal.z;
  result += sh.L11.rgb * Y11 * normal.x;

  result += sh.L2_2.rgb * Y2_2 * (normal.x * normal.y);
  result += sh.L2_1.rgb * Y2_1 * (normal.y * normal.z);
  result += sh.L20.rgb * Y20 * (3.0f * normal.z * normal.z - 1.0f);
  result += sh.L21.rgb * Y21 * (normal.x * normal.z);
  result += sh.L22.rgb * Y22 * (normal.x * normal.x - normal.y * normal.y);

  return max(result, 0.0.xxx);
}

// Compute the static IBL contribution.
// TODO: Transform the normal vector to the sky-view LUT space.
vec3 evaluateIBL(float iblIndex, SurfaceProperties gBuffer, AtmosphereParams atmo, float diffAO)
{
  float nDotV = max(dot(gBuffer.normal, gBuffer.view), 0.0);
  vec3 r = normalize(reflect(-gBuffer.view, gBuffer.normal));
  vec3 diffuseAlbedo = gBuffer.albedo * (1.0 - gBuffer.metalness);
  
  vec3 irradiance = evaluateDiffSH(compactedSH[uint(iblIndex)], normalToSky(gBuffer.normal, atmo)) * diffuseAlbedo * diffAO;

  float lod = MAX_MIP * gBuffer.roughness;
  vec3 specularDecomp = textureLod(radianceMap, vec4(r, iblIndex), lod).rgb;
  vec2 splitSums = texture(brdfLookUp, vec2(nDotV, gBuffer.roughness)).rg;

  vec3 mappedF0 = mix(gBuffer.dielectricF0, gBuffer.metallicF0, gBuffer.metalness);
  vec3 brdf = mix(splitSums.xxx, splitSums.yyy, mappedF0);

  vec3 specularRadiance = brdf * specularDecomp
                        * computeSpecularAO(nDotV, diffAO, gBuffer.roughness);

  return irradiance + specularRadiance;
  return irradiance;
}

//------------------------------------------------------------------------------
// Joined bilateral upsample from:
// - https://www.shadertoy.com/view/3dK3zR
// - https://bartwronski.com/2019/09/22/local-linear-models-guided-filter/
// - https://johanneskopf.de/publications/jbu/paper/FinalPaper_0185.pdf
//------------------------------------------------------------------------------
float gaussian(float sigma, float x)
{
  return exp(-(x * x) / (2.0 * sigma * sigma));
}

float getHBAO(sampler2D gDepth, sampler2D hbaoTexture, vec2 gBufferCoords)
{
  vec2 halfTexSize = vec2(textureSize(gDepth, 1).xy);
  vec2 halfTexelSize = 1.0.xx / halfTexSize;
  vec2 pixel = gBufferCoords * halfTexSize + 0.5.xx;
  vec2 f = fract(pixel);
  pixel = (floor(pixel) / halfTexSize) - (halfTexelSize / 2.0);

  float fullResDepth = textureLod(gDepth, gBufferCoords, 0.0).r;

  float halfResDepth[4];
  halfResDepth[0] = textureLodOffset(gDepth, pixel, 1.0, ivec2(0, 0)).r;
  halfResDepth[1] = textureLodOffset(gDepth, pixel, 1.0, ivec2(0, 1)).r;
  halfResDepth[2] = textureLodOffset(gDepth, pixel, 1.0, ivec2(1, 0)).r;
  halfResDepth[3] = textureLodOffset(gDepth, pixel, 1.0, ivec2(1, 1)).r;

  float sigmaV = 0.006;
  float weights[4];
  weights[0] = gaussian(sigmaV, abs(fullResDepth - halfResDepth[0])) * (1.0 - f.x) * (1.0 - f.y);
  weights[1] = gaussian(sigmaV, abs(fullResDepth - halfResDepth[1])) * (1.0 - f.x) * f.y;
  weights[2] = gaussian(sigmaV, abs(fullResDepth - halfResDepth[2])) * f.x * (1.0 - f.y);
  weights[3] = gaussian(sigmaV, abs(fullResDepth - halfResDepth[3])) * f.x * f.y;

  float halfResEffect[4];
  halfResEffect[0] = textureOffset(hbaoTexture, pixel, ivec2(0, 0)).r;
  halfResEffect[1] = textureOffset(hbaoTexture, pixel, ivec2(0, 1)).r;
  halfResEffect[2] = textureOffset(hbaoTexture, pixel, ivec2(1, 0)).r;
  halfResEffect[3] = textureOffset(hbaoTexture, pixel, ivec2(1, 1)).r;

  float result = halfResEffect[0] * weights[0] + halfResEffect[1] * weights[1]
               + halfResEffect[2] * weights[2] + halfResEffect[3] * weights[3];

  float weightSum = max(weights[0] + weights[1] + weights[2] + weights[3], 1e-4);
  return result / weightSum;
}
