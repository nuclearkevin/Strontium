#type compute
#version 460 core
/*
 * Shader for the directional light widget in the Strontium editor.
 */

#define PI 3.141592654

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) restrict writeonly uniform image2D outImage;

layout(location = 0) uniform mat4 u_invVP;
layout(location = 1) uniform vec3 u_camPos;
layout(location = 2) uniform vec3 u_lightDir;

float rayIntersectSphere(vec3 ro, vec3 rd, float rad)
{
  float b = dot(ro, rd);
  float c = dot(ro, ro) - rad * rad;
  if (c > 0.0f && b > 0.0)
    return -1.0;

  float discr = b * b - c;
  if (discr < 0.0)
    return -1.0;

  // Special case: inside sphere, use far discriminant
  if (discr > b * b)
    return (-b + sqrt(discr));

  return -b - sqrt(discr);
}

void main()
{
  ivec2 invoke = ivec2(gl_GlobalInvocationID.xy);
  vec2 uvs = (vec2(invoke) + 0.5.xx) / vec2(imageSize(outImage)).xy;

  vec4 clipPos = u_invVP * vec4(2.0 * uvs - 1.0, 1.0, 1.0);
  vec3 fragPos = clipPos.xyz / clipPos.w;

  // Raytrace a sphere.
  vec3 rayDir = normalize(fragPos - u_camPos);
  float distToSphere = rayIntersectSphere(u_camPos, rayDir, 1.0);

  vec3 colour = vec3(0.1);
  if (distToSphere > -1.0)
  {
    // Get the intersection normal (sphere is at the coordiante center).
    vec3 sphereNorm = normalize(u_camPos + distToSphere * rayDir);

    float nDotL = max(dot(-u_lightDir, sphereNorm), 0.0);
    colour = vec3(nDotL / PI);
  }

  imageStore(outImage, invoke, vec4(colour, 1.0));
}
