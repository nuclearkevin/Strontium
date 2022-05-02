#type compute
#version 460 core
/*
 * A compute shader to compute physically-based screenspace godrays. Based off
 * of:
 * https://www.alexandre-pestana.com/volumetric-lights/
*/

layout(local_size_x = 8, local_size_y = 8) in;

#define PI 3.141592654
#define NUM_CASCADES 4
#define EPSILON 1e-6

// TODO: Get density from a volume texture instead.
struct ScatteringParams
{
  vec3 scattering;
  vec3 emission;
  float phase;
  float extinction;
};

// The output texture for the godrays.
layout(rgba16f, binding = 0) restrict writeonly uniform image2D godrayOut;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D noise;

layout(binding = 2) uniform sampler3D scatExtinction;
layout(binding = 3) uniform sampler3D emissionPhase;

// TODO: Texture arrays.
layout(binding = 4) uniform sampler2D cascadeMaps[NUM_CASCADES];

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

layout(std140, binding = 1) uniform GodrayBlock
{
  vec4[2] padding;
  vec4 u_lightDirMiePhase; // Light direction (x, y, z) and the Mie phase (w).
  vec4 u_lightColourIntensity; // Light colour (x, y, z) and intensity (w).
  vec4 u_godrayParams; // Blur direction (x, y), number of steps (z).
  ivec4 u_numFogVolumes; // Number of OBB fog volumes (x). y, z and w are unused.
};

layout(std140, binding = 2) uniform CascadedShadowBlock
{
  mat4 u_lightVP[NUM_CASCADES];
  vec4 u_cascadeData[NUM_CASCADES]; // Cascade split distance (x). y, z and w are unused.
  vec4 u_shadowParams; // The constant bias (x), normal bias (y), the minimum PCF radius (z) and the cascade blend fraction (w).
};

vec4 computeGodrays(ivec2 coords, ivec2 outImageSize, sampler2D depthTex);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  ivec2 outImageSize = ivec2(imageSize(godrayOut).xy);

  // Quit early for threads that aren't in bounds of the screen.
  if (any(greaterThanEqual(invoke, imageSize(godrayOut).xy)))
    return;

  vec4 result = computeGodrays(invoke, outImageSize, gDepth);

  imageStore(godrayOut, invoke, max(result, 0.0.xxxx));
}

// Sample a dithering function.
vec4 sampleDither(ivec2 coords)
{
  return texture(noise, (vec2(coords) + 0.5.xx) / vec2(textureSize(noise, 0).xy));
}

vec4 fastTricubic(sampler3D tex, vec3 coord);

ScatteringParams getScatteringParams(vec3 position, float maxT, vec2 uv, vec3 dither)
{
  ScatteringParams outParams;

  float z = length(position - u_camPosition);
  z /= maxT;
  z = pow(z, 1.0 / 2.0);

  vec3 volumeUVW = vec3(uv, z);
  volumeUVW += (2.0 * dither - 1.0.xxx) / vec3(textureSize(scatExtinction, 0).xyz);

  vec4 se = fastTricubic(scatExtinction, volumeUVW);
  vec4 ep = fastTricubic(emissionPhase, volumeUVW);

  outParams.scattering = se.rgb;
  outParams.extinction = se.a;
  outParams.emission = ep.rgb;
  outParams.phase = ep.a;

  return outParams;
}

// Decodes the worldspace position of the fragment from depth.
vec3 decodePosition(ivec2 coords, ivec2 outImageSize, sampler2D depthMap, mat4 invVP)
{
  vec2 texCoords = (vec2(coords) + 0.5.xx) / vec2(outImageSize);
  float depth = textureLod(depthMap, texCoords, 1.0).r;
  vec3 clipCoords = 2.0 * vec3(texCoords, depth) - 1.0.xxx;
  vec4 temp = invVP * vec4(clipCoords, 1.0);
  return temp.xyz / temp.w;
}

float getMiePhase(float cosTheta, float g)
{
  const float scale = 3.0 / (8.0 * PI);

  float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
  float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
}

float cascadedShadow(vec3 samplePos)
{
  vec4 clipSpacePos = u_viewMatrix * vec4(samplePos, 1.0);
  float shadowFactor = 1.0;
  for (uint i = 0; i < NUM_CASCADES; i++)
  {
    if (-clipSpacePos.z <= (u_cascadeData[i].x))
    {
      vec4 lightClipPos = u_lightVP[i] * vec4(samplePos, 1.0);
      vec3 projCoords = lightClipPos.xyz / lightClipPos.w;
      projCoords = 0.5 * projCoords + 0.5;

      shadowFactor = float(texture(cascadeMaps[i], projCoords.xy).r >= projCoords.z);
      break;
    }
  }

  return shadowFactor;
}

vec4 computeGodrays(ivec2 coords, ivec2 outImageSize, sampler2D depthTex)
{
  const uint numSteps = uint(u_godrayParams.z);

  vec4 dither = sampleDither(coords);

  vec3 endPos = decodePosition(coords, outImageSize, depthTex, u_invViewProjMatrix);
  float rayT = length(endPos - u_camPosition) - EPSILON;
  float dt = rayT / max(float(numSteps) - 1.0, 1.0);
  vec3 rayDir = (endPos - u_camPosition) / rayT;
  vec3 startPos = u_camPosition + rayDir * dt * dither.a;
  rayT = length(endPos - startPos) - EPSILON;
  dt = rayT / max(float(numSteps) - 1.0, 1.0);

  vec2 uv = (vec2(coords) + 0.5.xx) / vec2(imageSize(godrayOut).xy);
  vec4 temp = u_invViewProjMatrix * vec4(2.0 * uv - 1.0, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  float maxT = length(worldSpaceMax - u_camPosition);

  vec3 luminance = 0.0.xxx;
  vec3 transmittance = 1.0.xxx;
  float t = 0.0;
  for (uint i = 0; i < numSteps; i++)
  {
    vec3 samplePos = startPos + t * rayDir;

    ScatteringParams params = getScatteringParams(samplePos, maxT, uv, dither.rgb);
    float phaseFunction = getMiePhase(dot(rayDir, u_lightDirMiePhase.xyz), params.phase);
    vec3 mieScattering = params.scattering;
    vec3 extinction = params.extinction.xxx;
    vec3 voxelAlbedo = mieScattering / extinction;

    float visibility = cascadedShadow(samplePos);

    vec3 lScatt = voxelAlbedo * phaseFunction * visibility;
    vec3 sampleTransmittance = exp(-dt * extinction);
    vec3 scatteringIntegral = (lScatt - lScatt * sampleTransmittance) / extinction;

    luminance += max(scatteringIntegral * transmittance, 0.0);
    luminance += params.emission * transmittance;
    transmittance *= min(sampleTransmittance, 1.0);
    t += dt;
  }

  luminance *= u_lightColourIntensity.xyz * u_lightColourIntensity.w;
  return vec4(luminance, dot(vec3(1.0 / 3.0), transmittance));
}

// The code below follows this license.
/*--------------------------------------------------------------------------*\
Copyright (c) 2008-2009, Danny Ruijters. All rights reserved.
http://www.dannyruijters.nl/cubicinterpolation/
This file is part of CUDA Cubic B-Spline Interpolation (CI).
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
*  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
*  Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
*  Neither the name of the copyright holders nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
The views and conclusions contained in the software and documentation are
those of the authors and should not be interpreted as representing official
policies, either expressed or implied.
When using this code in a scientific project, please cite one or all of the
following papers:
*  Daniel Ruijters and Philippe Thévenaz,
   GPU Prefilter for Accurate Cubic B-Spline Interpolation,
   The Computer Journal, vol. 55, no. 1, pp. 15-20, January 2012.
   http://dannyruijters.nl/docs/cudaPrefilter3.pdf
*  Daniel Ruijters, Bart M. ter Haar Romeny, and Paul Suetens,
   Efficient GPU-Based Texture Interpolation using Uniform B-Splines,
   Journal of Graphics Tools, vol. 13, no. 4, pp. 61-69, 2008.
\*--------------------------------------------------------------------------*/

vec4 fastTricubic(sampler3D tex, vec3 coord)
{
  // shift the coordinate from [0,1] to [-0.5, nrOfVoxels-0.5]
  vec3 nrOfVoxels = vec3(textureSize(tex, 0));
  vec3 coord_grid = coord * nrOfVoxels - 0.5;
  vec3 index = floor(coord_grid);
  vec3 fraction = coord_grid - index;
  vec3 one_frac = 1.0 - fraction;

  vec3 w0 = 1.0/6.0 * one_frac*one_frac*one_frac;
  vec3 w1 = 2.0/3.0 - 0.5 * fraction*fraction*(2.0-fraction);
  vec3 w2 = 2.0/3.0 - 0.5 * one_frac*one_frac*(2.0-one_frac);
  vec3 w3 = 1.0/6.0 * fraction*fraction*fraction;

  vec3 g0 = w0 + w1;
  vec3 g1 = w2 + w3;
  vec3 mult = 1.0 / nrOfVoxels;
  vec3 h0 = mult * ((w1 / g0) - 0.5 + index);  //h0 = w1/g0 - 1, move from [-0.5, nrOfVoxels-0.5] to [0,1]
  vec3 h1 = mult * ((w3 / g1) + 1.5 + index);  //h1 = w3/g1 + 1, move from [-0.5, nrOfVoxels-0.5] to [0,1]

  // fetch the eight linear interpolations
  // weighting and fetching is interleaved for performance and stability reasons
  vec4 tex000 = texture(tex, h0);
  vec4 tex100 = texture(tex, vec3(h1.x, h0.y, h0.z));
  tex000 = mix(tex100, tex000, g0.x);  //weigh along the x-direction
  vec4 tex010 = texture(tex, vec3(h0.x, h1.y, h0.z));
  vec4 tex110 = texture(tex, vec3(h1.x, h1.y, h0.z));
  tex010 = mix(tex110, tex010, g0.x);  //weigh along the x-direction
  tex000 = mix(tex010, tex000, g0.y);  //weigh along the y-direction
  vec4 tex001 = texture(tex, vec3(h0.x, h0.y, h1.z));
  vec4 tex101 = texture(tex, vec3(h1.x, h0.y, h1.z));
  tex001 = mix(tex101, tex001, g0.x);  //weigh along the x-direction
  vec4 tex011 = texture(tex, vec3(h0.x, h1.y, h1.z));
  vec4 tex111 = texture(tex, h1);
  tex011 = mix(tex111, tex011, g0.x);  //weigh along the x-direction
  tex001 = mix(tex011, tex001, g0.y);  //weigh along the y-direction

  return mix(tex001, tex000, g0.z);  //weigh along the z-direction
}
