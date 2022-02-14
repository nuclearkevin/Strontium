#type common
#version 460 core
/*
 * A static mesh shader program for the geometry pass.
 */

struct MaterialData
{
  vec4 mRAE; // Metallic (r), roughness (g), AO (b) and emission (a);
  vec4 albedoReflectance; // Albedo (r, g, b) and reflectance (a);
};

struct EntityData
{
  mat4 u_transform;
  vec4 u_maskID; // Is this entity selected (x), and the entity ID (y). Z and w are unused.
  MaterialData u_material;
};

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFar; // Near plane (x), far plane (y). z and w are unused.
};

// An index for fetching data from the transform and editor SSBOs.
layout(std140, binding = 1) uniform PerDrawBlock
{
  int u_drawData; // Transform ID (x). Y, z and w are unused.
};

// The per-entity data.
layout(std140, binding = 0) readonly buffer EntityBlock
{
  EntityData u_entityData[];
};

#type vertex
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in vec3 vBitangent;

// Vertex properties for shading.
out VERT_OUT
{
  vec3 fNormal;
  vec2 fTexCoords;
  mat3 fTBN;
  vec2 fMaskID;
  MaterialData fMaterialData;
} vertOut;

void main()
{
  // Compute the index of this draw into the global buffer.
  const int index = gl_InstanceID + u_drawData;

  // Fetch the transform from the global buffer.
  const mat4 modelMatrix = u_entityData[index].u_transform;

  // Tangent to world matrix calculation.
  vec3 T = normalize(vec3(modelMatrix * vec4(vTangent, 0.0)));
  vec3 N = normalize(vec3(modelMatrix * vec4(vNormal, 0.0)));
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);

  gl_Position = u_projMatrix * u_viewMatrix * modelMatrix * vPosition;
  vertOut.fNormal = N;
  vertOut.fTexCoords = vTexCoord;
  vertOut.fTBN = mat3(T, B, N);
  vertOut.fMaskID = u_entityData[index].u_maskID.xy;
  vertOut.fMaterialData = u_entityData[index].u_material;
}

#type fragment
layout (location = 0) out vec4 gNormal; // z and w components unused.
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec4 gMatProp;
layout (location = 3) out vec4 gIDMaskColour; // This should be a 2-component buffer...

in VERT_OUT
{
	vec3 fNormal;
  vec2 fTexCoords;
	mat3 fTBN;
  vec2 fMaskID;
  MaterialData fMaterialData;
} fragIn;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D roughnessMap;
layout(binding = 3) uniform sampler2D metallicMap;
layout(binding = 4) uniform sampler2D aOcclusionMap;
layout(binding = 5) uniform sampler2D specF0Map;

vec3 getNormal(sampler2D normalMap, mat3 tbn, vec2 texCoords)
{
  vec3 n = texture(normalMap, texCoords).xyz;
  n = normalize(2.0 * n - 1.0.xxx);
  n = tbn * n;
  return normalize(n);
}

// Fast octahedron normal vector encoding.
// https://jcgt.org/published/0003/02/01/
vec2 signNotZero(vec2 v)
{
  return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}
// Assume normalized input. Output is on [-1, 1] for each component.
vec2 encodeNormal(vec3 v)
{
  // Project the sphere onto the octahedron, and then onto the xy plane
  vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
  // Reflect the folds of the lower hemisphere over the diagonals
  return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}

void main()
{
  vec4 albedo = texture(albedoMap, fragIn.fTexCoords);
  vec4 albedoReflectance = fragIn.fMaterialData.albedoReflectance;
  vec4 mrae = fragIn.fMaterialData.mRAE;
  if (albedo.a < 1e-4)
    discard;

  gAlbedo = vec4(pow(albedo.rgb * albedoReflectance.rgb, vec3(2.2)), 1.0);
  gAlbedo.a = texture(specF0Map, fragIn.fTexCoords).r * albedoReflectance.a;
  gNormal.rg = encodeNormal(getNormal(normalMap, fragIn.fTBN, fragIn.fTexCoords));
  gNormal.ba = 1.0.xx;

  gMatProp.r = texture(metallicMap, fragIn.fTexCoords).r * mrae.r;
  gMatProp.g = texture(roughnessMap, fragIn.fTexCoords).r * mrae.g;
  gMatProp.b = texture(aOcclusionMap, fragIn.fTexCoords).r * mrae.b;
  gMatProp.a = mrae.a;

  gIDMaskColour = vec4(fragIn.fMaskID.xxx, fragIn.fMaskID.y);
}
