#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of up to 8
 * rectangular polygonal lights in deferred shading.
 */

#define MAX_NUM_RECT_LIGHTS 64

layout(local_size_x = 16, local_size_y = 16) in;

struct RectAreaLight
{
  vec4 colourIntensity; // Colour (x, y, z) and intensity (w).
  // Points of the rectangular area light (x, y, z). 
  // points[0].w > 0 indicates the light is two-sided, one-sided otherwise.
  // points[1].w is the culling radius.
  vec4 points[4];
};

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

// Uniforms for the geometry buffer.
layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMatProp;

// Uniforms for the LTC LUTs.
layout(binding = 4) uniform sampler2D u_LTC1;
layout(binding = 5) uniform sampler2D u_LTC2;

// The lighting buffer.
layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

// Light indices.
layout(std430, binding = 0) readonly buffer LightIndices
{
  int indices[];
};

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y), gamma (z). w is unused.
};

// Rectangular area light uniforms.
layout(std140, binding = 1) uniform RectAreaBlock
{
  RectAreaLight rALights[MAX_NUM_RECT_LIGHTS];
  ivec4 u_lightingSettings; // Number of rectangular area lights (x) with a maximum of 64. y, z and w are unused.
};

// Decode the g-buffer.
SurfaceProperties decodeGBuffer(vec2 gBufferUVs, ivec2 gBufferTexel);

// Evaluate a single rectangular area light.
vec3 evaluateRectAreaLight(RectAreaLight light, SurfaceProperties props);

// Some constants for sampling the LUTs.
const vec2 ltcLUT2Size = vec2(textureSize(u_LTC2, 0).xy);
const vec2 ltcLUT2Scale = (ltcLUT2Size - 1.0.xx) / ltcLUT2Size;
const vec2 ltcLUT2Bias = 0.5.xx / ltcLUT2Size;

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  ivec2 gBufferSize = ivec2(textureSize(gDepth, 0).xy);
  const ivec2 totalTiles = ivec2(gl_NumWorkGroups.xy); // Same as the total number of tiles.
  const ivec2 currentTile = ivec2(gl_WorkGroupID.xy); // The current tile.
  const uint tileOffset = (totalTiles.x * currentTile.y + currentTile.x) * MAX_NUM_RECT_LIGHTS; // Offset into the tile buffer.

  // Quit early for threads that aren't in bounds of the screen.
  if (any(greaterThanEqual(invoke, gBufferSize)))
    return;

  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(textureSize(gDepth, 0).xy);

  if (texelFetch(gDepth, invoke, 0).r >= (1.0 - 1e-6))
    return;

  SurfaceProperties gBuffer = decodeGBuffer(gBufferUVs, invoke);

  // Loop over the lights and compute the radiance contribution for everything
  // that isn't the shadowed light.
  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;
  for (int i = 0; i < u_lightingSettings.x; i++)
  {
    int lightIndex = indices[tileOffset + i];

    // If the buffer terminates in -1, quit. No more lights to add to this
    // fragment.
    if (lightIndex < 0)
      break;

    totalRadiance += max(evaluateRectAreaLight(rALights[lightIndex], gBuffer), 0.0.xxx);
  }

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

  vec3 mra = texelFetch(gMatProp, gBufferTexel, 0).rgb;
  decoded.metalness = mra.r;
  decoded.roughness = mra.g;
  decoded.ao = mra.b;

  // Remap material properties.
  vec4 albedoReflectance = texelFetch(gAlbedo, gBufferTexel, 0).rgba;
  decoded.albedo = albedoReflectance.rgb;
  decoded.dielectricF0 = 0.16 * albedoReflectance.aaa * albedoReflectance.aaa;
  decoded.metallicF0 = decoded.albedo * decoded.metalness;

  return decoded;
}

// Not completely correct but it removes tiling artifacts.
float computeCutoff(vec3 lPos, vec3 pos, float outerRadius, float innerRadius)
{
  vec3 diff = pos - lPos;
  float distSquared = dot(diff, diff);
  float invLightRadius = 1.0 / outerRadius;
  float factor = distSquared * invLightRadius * invLightRadius;

  return max(1.0 - factor * factor, 0.0);
}

// Compute the vector form factor.
vec3 vectorFormFactor(vec3 v1, vec3 v2)
{
  float x = dot(v1, v2);
  float y = abs(x);
  float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
  float b = 3.4175940 + (4.1616724 + y) * y;
  float v = a / b;
  float thetaSinTheta = (x > 0.0) ? v : 0.5 * inversesqrt(max(1.0 - x * x, 1e-7)) - v;
  return cross(v1, v2) * thetaSinTheta;
}

vec3 evaluateLTC(RectAreaLight light, SurfaceProperties props, mat3 invCosM)
{
  bool twoSided = light.points[0].w > 0;

  // Construct orthonormal basis around the surface normal.
  vec3 T1, T2;
  T1 = normalize(props.view - props.normal * dot(props.view, props.normal));
  T2 = cross(props.normal, T1);
  // Pre-multiply the transformations.
  invCosM = invCosM * transpose(mat3(T1, T2, props.normal));

  // Convert from the fragment local LTC space to the origin.
  vec3 rectPoints[4];
  rectPoints[0] = invCosM * (light.points[0].xyz - props.position);
  rectPoints[1] = invCosM * (light.points[1].xyz - props.position);
  rectPoints[2] = invCosM * (light.points[2].xyz - props.position);
  rectPoints[3] = invCosM * (light.points[3].xyz - props.position);

  // Convert to local cosine-weighted space.
  rectPoints[0] = normalize(rectPoints[0]);
  rectPoints[1] = normalize(rectPoints[1]);
  rectPoints[2] = normalize(rectPoints[2]);
  rectPoints[3] = normalize(rectPoints[3]);

  // Integrate to obtain the net vector form factor.
  vec3 integral = vec3(0.0);
  integral += vectorFormFactor(rectPoints[0], rectPoints[1]);
  integral += vectorFormFactor(rectPoints[1], rectPoints[2]);
  integral += vectorFormFactor(rectPoints[2], rectPoints[3]);
  integral += vectorFormFactor(rectPoints[3], rectPoints[0]);
  // Scalar form factor in the direction of norm(integral).
  float formFactor = length(integral);

  // Check to see if the light is behind the current fragment.
  vec3 lightDir = light.points[0].xyz - props.position;
  vec3 lightNormal = cross(light.points[1].xyz - light.points[0].xyz, light.points[3].xyz - light.points[0].xyz);
  bool behind = dot(lightDir, lightNormal) < 0.0;
  float z = integral.z / formFactor * mix(1.0, -1.0, float(behind));

  // Fetch the horizon sphere from the LUT.
  vec2 uv = vec2(z * 0.5 + 0.5, formFactor);
  uv = uv * ltcLUT2Scale + ltcLUT2Bias;
  float sphere = texture(u_LTC2, uv).w;

  return 1.0.xxx * (sphere * formFactor * float(!behind || twoSided));
}

// Evaluate a single rectangular area light.
// Adapted to approximate the Filament BRDF.
vec3 evaluateRectAreaLight(RectAreaLight light, SurfaceProperties props)
{
  float nDotV = clamp(dot(props.normal, props.view), 0.0, 1.0);

  // Use roughness and sqrt(1-cos_theta) to sample the LUTs.
  float clampedRoughness = max(props.roughness, 0.045);
  float actualRoughness = clampedRoughness * clampedRoughness;
  vec2 uv = vec2(clampedRoughness, sqrt(1.0f - nDotV));
  uv = uv * ltcLUT2Scale + ltcLUT2Bias;

  // Get the four parameters of invCosM.
  vec4 t1 = texture(u_LTC1, uv);  
  // Get the two parameters required for the GGX specular calculation.
  vec4 t2 = texture(u_LTC2, uv);

  vec3 diffuseBRDF = evaluateLTC(light, props, mat3(1.0)) * props.albedo * (1.0 - props.metalness);
  vec3 specularBRDF = evaluateLTC(light, props, mat3(vec3(t1.x, 0.0, t1.y), vec3(0.0, 1.0, 0.0), vec3(t1.z, 0.0, t1.w)));

  vec3 dielectricSpecularBRDF = specularBRDF * (props.dielectricF0 * t2.x + (1.0.xxx - props.dielectricF0) * t2.y);
  vec3 metallicSpecularBRDF = specularBRDF * (props.metallicF0 * t2.x + (1.0.xxx - props.metallicF0) * t2.y);
  vec3 brdf = mix(dielectricSpecularBRDF, metallicSpecularBRDF, props.metalness) + diffuseBRDF;

  vec3 center = 0.25 * (light.points[0].xyz + light.points[1].xyz + light.points[2].xyz + light.points[3].xyz);
  float attenuation = computeCutoff(center, props.position, light.points[1].w, light.points[2].w);
  attenuation = mix(attenuation, 1.0, float(light.points[3].w < 1.0));

  return light.colourIntensity.rgb * light.colourIntensity.a * brdf 
         * attenuation;
}