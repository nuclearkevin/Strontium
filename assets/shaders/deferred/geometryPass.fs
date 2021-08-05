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

// The material properties.
layout(std140, binding = 1) uniform MaterialBlock
{
  mat4 u_modelMatrix;
  vec4 u_MRAE; // Metallic (r), roughness (g), AO (b) and emission (a);
	vec4 u_albedoF0; // Albedo (r, g, b) and F0 (a);
};

// Editor block.
layout(std140, binding = 2) uniform EditorBlock
{
	vec4 maskColourID; // Mask colour (r, g, b) and the entity ID (a).
};

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform sampler2D aOcclusionMap;
uniform sampler2D specF0Map;

void main()
{
  gPosition = vec4(fragIn.fPosition, 1.0);
  gNormal = vec4(fragIn.fTBN * (texture(normalMap, fragIn.fTexCoords).xyz * 2.0 - 1.0), 1.0);
  gAlbedo = vec4(pow(texture(albedoMap, fragIn.fTexCoords).rgb * u_albedoF0.rgb, vec3(2.2)), 1.0);
	gAlbedo.a = texture(specF0Map, fragIn.fTexCoords).r * u_albedoF0.a;

  gMatProp.r = texture(metallicMap, fragIn.fTexCoords).r * u_MRAE.r;
  gMatProp.g = texture(roughnessMap, fragIn.fTexCoords).r * u_MRAE.g;
  gMatProp.b = texture(aOcclusionMap, fragIn.fTexCoords).r * u_MRAE.b;
  gMatProp.a = u_MRAE.a;

	gIDMaskColour = maskColourID;
}
