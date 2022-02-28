#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of image-based lighting.
 */

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

  vec3 position;
  vec3 normal;
  vec3 view;
};

// GBuffer.
layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMatProp;

// TODO: Spherical harmonics instead of a cubemap.
layout(binding = 4) uniform samplerCubeArray irradianceMap;
layout(binding = 5) uniform samplerCubeArray radianceMap;
layout(binding = 6) uniform sampler2D brdfLookUp;
layout(binding = 7) uniform sampler2D hbaoTexture;

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
  vec4 u_iblParams; // IBL index (x), IBL intensity (y), use HBAO (z). w is unused.
};

// Decode the g-buffer.
SurfaceProperties decodeGBuffer(vec2 gBufferUVs);

// Compute the static IBL contribution.
vec3 evaluateIBL(float iblIndex, SurfaceProperties gBuffer, float diffAO);

// Fetches and upsamples the horizon-based ambient occlusion.
float getHBAO(sampler2D gDepth, sampler2D hbaoTexture, vec2 gBufferCoords, vec2 nearFar);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(textureSize(gDepth, 0).xy);

  if (texture(gDepth, gBufferUVs).r >= 1.0)
    return;

  SurfaceProperties gBuffer = decodeGBuffer(gBufferUVs);

  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;

  float ao = gBuffer.ao;
  if (u_iblParams.z > 0.0)
    ao = min(getHBAO(gDepth, hbaoTexture, gBufferUVs, u_nearFar.xy), ao);

  totalRadiance += evaluateIBL(u_iblParams.x, gBuffer, ao) * u_iblParams.y;

  imageStore(lightingBuffer, invoke, vec4(totalRadiance, 1.0));
}

// Decodes the worldspace position of the fragment from depth.
vec3 decodePosition(vec2 texCoords, sampler2D depthMap, mat4 invMVP)
{
  float depth = texture(depthMap, texCoords).r;
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
vec3 decodeNormal(vec2 texCoords, sampler2D encodedNormals)
{
  vec2 e = texture(encodedNormals, texCoords).xy;
  vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
  if (v.z < 0)
    v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
  return normalize(v);
}

SurfaceProperties decodeGBuffer(vec2 gBufferUVs)
{
  SurfaceProperties decoded;
  decoded.position = decodePosition(gBufferUVs, gDepth, u_invViewProjMatrix);
  decoded.view = normalize(u_camPosition - decoded.position);
  decoded.normal = decodeNormal(gBufferUVs, gNormal);

  vec3 mra = texture(gMatProp, gBufferUVs).rgb;
  decoded.metalness = mra.r;
  decoded.roughness = mra.g;
  decoded.ao = mra.b;

  // Remap material properties.
  vec4 albedoReflectance = texture(gAlbedo, gBufferUVs).rgba;
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

// Compute the static IBL contribution.
vec3 evaluateIBL(float iblIndex, SurfaceProperties gBuffer, float diffAO)
{
  float nDotV = max(dot(gBuffer.normal, gBuffer.view), 0.0);
  vec3 r = normalize(reflect(-gBuffer.view, gBuffer.normal));
  vec3 diffuseAlbedo = gBuffer.albedo * (1.0 - gBuffer.metalness);

  vec3 irradiance = texture(irradianceMap, vec4(gBuffer.normal, iblIndex)).rgb
                    * diffuseAlbedo * diffAO;

  float lod = MAX_MIP * gBuffer.roughness;
  vec3 specularDecomp = textureLod(radianceMap, vec4(r, iblIndex), lod).rgb;
  vec2 splitSums = texture(brdfLookUp, vec2(nDotV, gBuffer.roughness)).rg;

  vec3 mappedF0 = mix(gBuffer.dielectricF0, gBuffer.metallicF0, gBuffer.metalness);
  vec3 brdf = mix(splitSums.xxx, splitSums.yyy, mappedF0);

  vec3 specularRadiance = brdf * specularDecomp
                        * computeSpecularAO(nDotV, diffAO, gBuffer.roughness);

  return irradiance + specularRadiance;
}

// https://stackoverflow.com/questions/51108596/linearize-depth
float linearizeDepth(float d, float zNear, float zFar)
{
  float zN = 2.0 * d - 1.0;
  return 2.0 * zNear * zFar / (zFar + zNear - zN * (zFar - zNear));
}

float getHBAO(sampler2D gDepth, sampler2D hbaoTexture, vec2 gBufferCoords,
              vec2 nearFar)
{
  float fullResDepth = linearizeDepth(textureLod(gDepth, gBufferCoords, 0.0).r,
                                                 nearFar.x, nearFar.y);

  float halfResDepth[4];
  halfResDepth[0] = linearizeDepth(textureLodOffset(gDepth, gBufferCoords, 1.0,
                                                    ivec2(0, 0)).r, nearFar.x,
                                                    nearFar.y);
  halfResDepth[1] = linearizeDepth(textureLodOffset(gDepth, gBufferCoords, 1.0,
                                                    ivec2(1, 0)).r, nearFar.x,
                                                    nearFar.y);
  halfResDepth[2] = linearizeDepth(textureLodOffset(gDepth, gBufferCoords, 1.0,
                                                    ivec2(0, 1)).r, nearFar.x,
                                                    nearFar.y);
  halfResDepth[3] = linearizeDepth(textureLodOffset(gDepth, gBufferCoords, 1.0,
                                                    ivec2(1, 1)).r, nearFar.x,
                                                    nearFar.y);

  float weights[4];
  weights[0] = max(0.0, 1.0 - (0.05 * abs(halfResDepth[0] - fullResDepth)));
  weights[1] = max(0.0, 1.0 - (0.05 * abs(halfResDepth[1] - fullResDepth)));
  weights[2] = max(0.0, 1.0 - (0.05 * abs(halfResDepth[2] - fullResDepth)));
  weights[3] = max(0.0, 1.0 - (0.05 * abs(halfResDepth[3] - fullResDepth)));

  float halfResEffect[4];
  halfResEffect[0] = textureOffset(hbaoTexture, gBufferCoords, ivec2(0, 0)).r;
  halfResEffect[1] = textureOffset(hbaoTexture, gBufferCoords, ivec2(1, 0)).r;
  halfResEffect[2] = textureOffset(hbaoTexture, gBufferCoords, ivec2(0, 1)).r;
  halfResEffect[3] = textureOffset(hbaoTexture, gBufferCoords, ivec2(1, 1)).r;

  float result = halfResEffect[0] * weights[0] + halfResEffect[1] * weights[1]
               + halfResEffect[2] * weights[2] + halfResEffect[3] * weights[3];
  return result / max(weights[0] + weights[1] + weights[2] + weights[3], 1e-4);
}
