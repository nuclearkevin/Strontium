#version 440
/*
 * A fragment shader for the geometry pass in deferred rendering.
 */

layout (location = 4) out vec4 gPosition;
layout (location = 3) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 1) out vec4 gMatProp;
layout (location = 0) out vec4 gIDMaskColour;

in VERT_OUT
{
	vec3 fNormal;
	vec3 fPosition;
	vec3 fColour;
  vec2 fTexCoords;
	mat3 fTBN;
} fragIn;

uniform vec3 uAlbedo = vec3(1.0);
uniform float uMetallic = 1.0;
uniform float uRoughness = 1.0;
uniform float uAO = 1.0;
uniform float uID = 0.0;
uniform vec3 uMaskColour = vec3(0.0);

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform sampler2D aOcclusionMap;

void main()
{
  gPosition = vec4(fragIn.fPosition, 1.0);
  gNormal = vec4(fragIn.fTBN * (texture(normalMap, fragIn.fTexCoords).xyz * 2.0 - 1.0), 1.0);
  gAlbedo = vec4(pow(texture(albedoMap, fragIn.fTexCoords).rgb * uAlbedo, vec3(2.2)), 1.0);

  vec4 outMatProps = vec4(0.0);
  outMatProps.r = texture(metallicMap, fragIn.fTexCoords).r * uMetallic;
  outMatProps.g = texture(roughnessMap, fragIn.fTexCoords).r * uRoughness;
  outMatProps.b = texture(aOcclusionMap, fragIn.fTexCoords).r * uAO;
  outMatProps.a = 1.0;
  gMatProp = outMatProps;
	gIDMaskColour = vec4(uMaskColour, uID);
}
