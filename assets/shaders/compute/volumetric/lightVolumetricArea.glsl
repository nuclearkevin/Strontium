#type compute
#version 460 core

#define PI 3.141592654
#define MAX_NUM_RECT_LIGHTS 8

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

struct RectAreaLight
{
  vec4 colourIntensity; // Colour (x, y, z) and intensity (w).
  // Points of the rectangular area light (x, y, z). 
  // points[0].w > 0 indicates the light is two-sided, one-sided otherwise.
  vec4 points[4];
};

layout(rgba16f, binding = 0) restrict uniform image3D inScatExt;

layout(binding = 0) uniform sampler3D scatExtinction;
layout(binding = 1) uniform sampler3D emissionPhase; 
layout(binding = 2) uniform sampler2D noise; // Blue noise

// Uniforms for the LTC LUTs.
layout(binding = 3) uniform sampler2D u_LTC2;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

// Rectangular area light uniforms.
layout(std140, binding = 1) uniform RectAreaBlock
{
  RectAreaLight rALights[MAX_NUM_RECT_LIGHTS];
  ivec4 u_lightingSettings; // Number of rectangular area lights (x) with a maximum of 8. y, z and w are unused.
};

// Temporal AA parameters. TODO: jittered camera matrices.
layout(std140, binding = 3) uniform TemporalBlock
{
  mat4 u_previousView;
  mat4 u_previousProj;
  mat4 u_previousVP;
  mat4 u_prevInvViewProjMatrix;
  vec4 u_prevPosTime;
};

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  vec4 temporal = fract((u_prevPosTime.wwww + vec4(8.0, 9.0, 10.0, 11.0)) * 0.61803399);
  vec2 uv = (vec2(coords) + 0.5.xx) / vec2(textureSize(noise, 0).xy);
  return fract(texture(noise, uv) + temporal);
}

float getMiePhase(float cosTheta, float g)
{
  const float scale = 3.0 / (8.0 * PI);

  float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
  float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
}

// Some constants for sampling the LUTs.
const vec2 ltcLUT2Size = vec2(textureSize(u_LTC2, 0).xy);
const vec2 ltcLUT2Scale = (ltcLUT2Size - 1.0.xx) / ltcLUT2Size;
const vec2 ltcLUT2Bias = 0.5.xx / ltcLUT2Size;

// Evaluate a single rectangular area light.
vec3 evaluateRectAreaLight(RectAreaLight light, vec3 position, vec3 view, float g);

void main()
{
  ivec3 invoke = ivec3(gl_GlobalInvocationID.xyz);
  ivec3 numFroxels = ivec3(imageSize(inScatExt).xyz);

  if (any(greaterThanEqual(invoke, numFroxels)))
    return;

  vec3 dither = (2.0 * sampleDither(invoke.xy).rgb - 1.0.xxx) / vec3(numFroxels).xyz;
  vec2 uv = (vec2(invoke.xy) + 0.5.xx) / vec2(numFroxels.xy) + dither.xy;

  vec4 temp = u_invViewProjMatrix * vec4(2.0 * uv - 1.0.xx, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  vec3 direction = worldSpaceMax - u_camPosition;
  float w = (float(invoke.z) + 0.5) / float(numFroxels.z) + dither.z;
  vec3 worldSpacePostion = u_camPosition + direction * w * w;

  vec3 uvw = vec3(uv, w);

  vec4 se = texture(scatExtinction, uvw);
  vec4 ep = texture(emissionPhase, uvw);
  vec4 totalInScatExt = imageLoad(inScatExt, invoke);

  // Light the voxel. Just cascaded shadow maps and voxel emission for now.
  // TODO: Single sample along the shadowed light's direction to account for out scattering.
  vec3 mieScattering = se.xyz;
  vec3 extinction = se.www;
  vec3 voxelAlbedo = max(mieScattering / extinction, 0.0.xxx);

  vec3 totalRadiance = totalInScatExt.rgb;
  for (int i = 0; i < u_lightingSettings.x; i++)
  {
    totalRadiance += voxelAlbedo * evaluateRectAreaLight(rALights[i], worldSpacePostion, direction, ep.w);
  }

  imageStore(inScatExt, invoke, vec4(totalRadiance, totalInScatExt.w));
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

vec3 evaluateLTC(RectAreaLight light, vec3 position, vec3 view, vec3 averageLightDir, mat3 invCosM)
{
  bool twoSided = light.points[0].w > 0;

  // Construct orthonormal basis around the view vector.
  vec3 T1, T2;
  T1 = normalize(view - averageLightDir * dot(view, averageLightDir));
  T2 = cross(averageLightDir, T1);
  // Pre-multiply the transformations.
  invCosM = invCosM * transpose(mat3(T1, T2, averageLightDir));

  // Convert from the fragment local LTC space to the origin.
  vec3 rectPoints[4];
  rectPoints[0] = invCosM * (light.points[0].xyz - position);
  rectPoints[1] = invCosM * (light.points[1].xyz - position);
  rectPoints[2] = invCosM * (light.points[2].xyz - position);
  rectPoints[3] = invCosM * (light.points[3].xyz - position);

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
  vec3 lightDir = light.points[0].xyz - position;
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
vec3 evaluateRectAreaLight(RectAreaLight light, vec3 position, vec3 view, float g)
{
  vec3 avgLightDir = normalize(0.25 * (light.points[0].xyz + light.points[1].xyz + light.points[2].xyz + light.points[3].xyz) - position);

  // Need to multiply by \pi to account for all incident light as evaluateLTC divides by \pi implicitly.
  vec3 radiance = PI * evaluateLTC(light, position, view, avgLightDir, mat3(1.0));

  // Phase function.
  float phaseFunction = getMiePhase(dot(normalize(view), avgLightDir), g);

  return light.colourIntensity.rgb * light.colourIntensity.a * radiance * phaseFunction;
}