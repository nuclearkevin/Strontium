#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of volumetric effects,
 */

#define PI 3.141592654

layout(local_size_x = 8, local_size_y = 8) in;

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y). z and w are unused.
};

// Volumetric specific uniforms.
layout(std140, binding = 1) uniform VolumetricBlock
{
  ivec4 u_volumetricSettings1; // Bitmask for volumetric settings (x). y, z and w are unused.
};

layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D vGodrays; // Screen-space godrays.
layout(binding = 2) uniform sampler3D vHaze; // Aerial perspective.

// Get the volumetric haze effect.
vec3 getHaze(sampler3D hazeTex, sampler2D gDepth, vec2 gBufferCoords, vec2 nearFar);

// Get the screen-space godray effect.
vec3 getGodrays(sampler2D godrayTex, sampler2D gDepth, vec2 gBufferCoords, vec2 nearFar);

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(textureSize(gDepth, 0).xy);

  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;

  // Bit 1 is set, apply haze.
  if (u_volumetricSettings1.x & 1)
    totalRadiance += getHaze(vHaze, gDepth, gBufferUVs, u_nearFar.xy);

  // Bit 2 is set, apply screen-space godrays.
  if (u_volumetricSettings1.x & 2)
    totalRadiance += getGodrays(vGodrays, gDepth, gBufferUVs, u_nearFar.xy);

  imageStore(lightingBuffer, invoke, vec4(totalRadiance, 1.0));
}

//------------------------------------------------------------------------------
/*
 * The filter below follows this license:
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
*/

vec4 tricubicLookup(sampler3D tex, vec3 coord)
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
  vec3 h0 = mult * ((w1 / g0) - 0.5 + index);
  vec3 h1 = mult * ((w3 / g1) + 1.5 + index);

  // fetch the eight linear interpolations
  // weighting and fetching is interleaved for performance and stability reasons
  vec4 tex000 = texture(tex, h0).rgba;
  vec4 tex100 = texture(tex, vec3(h1.x, h0.y, h0.z)).rgba;
  tex000 = mix(tex100, tex000, g0.x);  //weigh along the x-direction
  vec4 tex010 = texture(tex, vec3(h0.x, h1.y, h0.z)).rgba;
  vec4 tex110 = texture(tex, vec3(h1.x, h1.y, h0.z)).rgba;
  tex010 = mix(tex110, tex010, g0.x);  //weigh along the x-direction
  tex000 = mix(tex010, tex000, g0.y);  //weigh along the y-direction
  vec4 tex001 = texture(tex, vec3(h0.x, h0.y, h1.z)).rgba;
  vec4 tex101 = texture(tex, vec3(h1.x, h0.y, h1.z)).rgba;
  tex001 = mix(tex101, tex001, g0.x);  //weigh along the x-direction
  vec4 tex011 = texture(tex, vec3(h0.x, h1.y, h1.z)).rgba;
  vec4 tex111 = texture(tex, h1).rgba;
  tex011 = mix(tex111, tex011, g0.x);  //weigh along the x-direction
  tex001 = mix(tex011, tex001, g0.y);  //weigh along the y-direction

  return mix(tex001, tex000, g0.z);  //weigh along the z-direction
}
//------------------------------------------------------------------------------
// END FILTER
//------------------------------------------------------------------------------

// Get the volumetric haze effect.
vec3 getHaze(sampler3D hazeTex, sampler2D gDepth, vec2 gBufferCoords, vec2 nearFar)
{
  vec3 froxel = 1.0.xxx / vec3(textureSize(hazeTex, 0).xyz);

  // Compute aerial perspective position using the view frustum near/far planes.
  float planeDelta = (nearFar.y - nearFar.x) * 1e-6 / froxel.z;
  float slice = textureLod(gDepth, gBufferCoords, 0.0).r / planeDelta;

  // Fade the slice out if it's close to the near plane.
  float weight = 1.0;
  if (slice < 0.5)
  {
    weight = clamp(2.0 * slice, 0.0, 1.0);
    slice = 0.5;
  }
  float w = sqrt(slice);

  // Spatial AA as described in [Patry2021]:
  // http://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html
  vec4 haze = tricubicLookup(hazeTex, vec3(gBufferCoords.xy, w));

  // Density AA as described in [Patry2021]:
  // http://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html
  haze.rgb *= haze.a;
  return haze.rgb * weight;
}

// https://stackoverflow.com/questions/51108596/linearize-depth
float linearizeDepth(float d, float zNear, float zFar)
{
  float zN = 2.0 * d - 1.0;
  return 2.0 * zNear * zFar / (zFar + zNear - zN * (zFar - zNear));
}

// A depth-based bilateral upsample for half-resolution effects.
vec4 depthUpsample(sampler2D depthPyramid, sampler2D effect, vec2 uv, vec2 nearfar)
{
  float fullResDepth = linearizeDepth(textureLod(depthPyramid, uv, 0.0).r, nearfar.x, nearfar.y);

  float halfResDepth[4];
  halfResDepth[0] = linearizeDepth(textureLodOffset(depthPyramid, uv, 1.0, ivec2(0, 0)).r, nearfar.x, nearfar.y);
  halfResDepth[1] = linearizeDepth(textureLodOffset(depthPyramid, uv, 1.0, ivec2(1, 0)).r, nearfar.x, nearfar.y);
  halfResDepth[2] = linearizeDepth(textureLodOffset(depthPyramid, uv, 1.0, ivec2(0, 1)).r, nearfar.x, nearfar.y);
  halfResDepth[3] = linearizeDepth(textureLodOffset(depthPyramid, uv, 1.0, ivec2(1, 1)).r, nearfar.x, nearfar.y);

  float weights[4];
  weights[0] = max(0.0, 1.0 - (0.05 * abs(halfResDepth[0] - fullResDepth)));
  weights[1] = max(0.0, 1.0 - (0.05 * abs(halfResDepth[1] - fullResDepth)));
  weights[2] = max(0.0, 1.0 - (0.05 * abs(halfResDepth[2] - fullResDepth)));
  weights[3] = max(0.0, 1.0 - (0.05 * abs(halfResDepth[3] - fullResDepth)));

  vec4 halfResEffect[4];
  halfResEffect[0] = textureOffset(effect, uv, ivec2(0, 0));
  halfResEffect[1] = textureOffset(effect, uv, ivec2(1, 0));
  halfResEffect[2] = textureOffset(effect, uv, ivec2(0, 1));
  halfResEffect[3] = textureOffset(effect, uv, ivec2(1, 1));

  vec4 result = halfResEffect[0] * weights[0] + halfResEffect[1] * weights[1]
              + halfResEffect[2] * weights[2] + halfResEffect[3] * weights[3];
  return result / max(weights[0] + weights[1] + weights[2] + weights[3], 1e-4);
}

// Get the screen-space godray effect.
vec3 getGodrays(sampler2D godrayTex, sampler2D gDepth, vec2 gBufferCoords, vec2 nearFar)
{
  vec4 godrays = depthUpsample(gDepth, godrayTex, gBufferCoords, nearFar.xy);

  // Density AA as described in [Patry2021]:
  // http://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html
  return godrays.rgb * godrays.a;
}
