#version 440
/*
 * A fragment shader for the geometry pass in deferred rendering.
 */

 layout (location = 0) out vec4 gPosition;
 layout (location = 1) out vec4 gNormal;
 layout (location = 2) out vec4 gAlbedo;
 layout (location = 3) out vec4 gMatProp;

 in VERT_OUT
 {
 	vec3 fNormal;
 	vec3 fPosition;
 	vec3 fColour;
  vec2 fTexCoords;
 	mat3 fTBN;
 } fragIn;

 layout (binding = 0) uniform sampler2D albedoMap;
 layout (binding = 1) uniform sampler2D normalMap;
 layout (binding = 2) uniform sampler2D roughnessMap;
 layout (binding = 3) uniform sampler2D metallicMap;
 layout (binding = 4) uniform sampler2D aOcclusionMap;

void main()
{
  gPosition = vec4(fragIn.fPosition, 1.0);
  gNormal = vec4(fragIn.fTBN * (texture(normalMap, fragIn.fTexCoords).xyz * 2.0 - 1.0), 1.0);
  gAlbedo = vec4(pow(texture(albedoMap, fragIn.fTexCoords).rgb, vec3(2.2)), 1.0);

  vec4 outMatProps = vec4(0.0);
  outMatProps.r = texture(metallicMap, fragIn.fTexCoords).r;
  outMatProps.g = texture(roughnessMap, fragIn.fTexCoords).r;
  outMatProps.b = texture(aOcclusionMap, fragIn.fTexCoords).r;
  outMatProps.a = 1.0;
  gMatProp = outMatProps;
}
