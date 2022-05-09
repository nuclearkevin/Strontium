#type compute
#version 460 core
/*
 * A compute shader for evaluating the lighting contribution of volumetric effects.
 */

#define PI 3.141592654

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) restrict uniform image2D lightingBuffer;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler3D froxels; // Volumetric final gather.

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
  ivec4 u_volumetricSettings; // Bitmask for volumetric settings (x). y, z and w are unused.
};

vec4 fastTricubic(sampler3D tex, vec3 coord);

// Decodes the worldspace position of the fragment from depth.
vec3 decodePosition(ivec2 coords, ivec2 outImageSize, sampler2D depthMap, mat4 invVP)
{
  vec2 texCoords = (vec2(coords) + 0.5.xx) / vec2(outImageSize);
  float depth = textureLod(depthMap, texCoords, 1.0).r;
  vec3 clipCoords = 2.0 * vec3(texCoords, depth) - 1.0.xxx;
  vec4 temp = invVP * vec4(clipCoords, 1.0);
  return temp.xyz / temp.w;
}

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  ivec2 outSize = imageSize(lightingBuffer).xy;
  vec2 gBufferUVs = (vec2(invoke) + 0.5.xx) / vec2(outSize);

  vec3 totalRadiance = imageLoad(lightingBuffer, invoke).rgb;

  vec3 worlSpacePos = decodePosition(invoke, outSize, gDepth, u_invViewProjMatrix);
  vec4 temp = u_invViewProjMatrix * vec4(2.0 * gBufferUVs - 1.0, 1.0, 1.0);
  vec3 worldSpaceMax = temp.xyz /= temp.w;
  float maxT = length(worldSpaceMax - u_camPosition);

  float z = length(worlSpacePos - u_camPosition);
  z /= maxT;
  z = pow(z, 1.0 / 2.0);

  vec3 volumeUVW = vec3(gBufferUVs, z);

  if ((u_volumetricSettings.x & (1 << 0)) != 0)
  {
    vec4 godrays = fastTricubic(froxels, volumeUVW);
    totalRadiance *= godrays.a;
    totalRadiance += godrays.rgb;
  }

  imageStore(lightingBuffer, invoke, vec4(totalRadiance, 1.0));
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
