#type common
#version 460 core
/*
 * A dynamic mesh shader program for the geometry pass.
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

struct VertexData
{
  vec4 normal;
  vec4 tangent;
  vec4 position; // Uncompressed position (x, y, z). w is padding.
  vec4 boneWeights; // Uncompressed bone weights.
  ivec4 boneIDs; // Bone IDs.
  vec4 texCoord; // UV coordinates (x, y). z and w are padding.
};

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  mat4 u_invViewProjMatrix;
  vec3 u_camPosition;
  vec4 u_nearFarGamma; // Near plane (x), far plane (y), gamma correction factor (z). w is unused.
};

#type vertex
#define MAX_BONES_PER_MODEL 512

// An index for fetching data from the transform and editor SSBOs.
layout(std140, binding = 1) uniform PerDrawBlock
{
  int u_drawData; // Transform ID (x). Y, z and w are unused.
};

layout(std140, binding = 0) readonly buffer VertexBuffer
{
  VertexData v_vertices[];
};

layout(std430, binding = 1) readonly buffer IndexBuffer
{
  uint v_indices[];
};

// The per-entity data.
layout(std140, binding = 2) readonly buffer EntityBlock
{
  EntityData u_entityData[];
};

layout(std140, binding = 3) readonly buffer BoneBlock
{
  mat4 u_boneMatrices[MAX_BONES_PER_MODEL];
};

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
  const uint vIndex = v_indices[gl_VertexID];
  const VertexData vertex = v_vertices[vIndex];

  // Compute the index of this draw into the global buffer.
  const int instance = gl_InstanceID + u_drawData;

  // Fetch the transform from the global buffer.
  const mat4 modelMatrix = u_entityData[instance].u_transform;

  // Skinning calculations.
  mat4 skinMatrix = vertex.boneIDs.x > -1 ? u_boneMatrices[vertex.boneIDs.x]
                                          * vertex.boneWeights.x
                                          : mat4(1.0);
  skinMatrix += u_boneMatrices[vertex.boneIDs.y] * vertex.boneWeights.y;
  skinMatrix += u_boneMatrices[vertex.boneIDs.z] * vertex.boneWeights.z;
  skinMatrix += u_boneMatrices[vertex.boneIDs.w] * vertex.boneWeights.w;

  mat4 worldSpaceMatrix = modelMatrix * skinMatrix;

  // Tangent to world matrix calculation.
  vec3 normal = normalize(vec3(modelMatrix * vertex.normal));
  vec3 tangent = normalize(vec3(modelMatrix * vertex.tangent));
  tangent = normalize(tangent - dot(tangent, normal) * normal);
  vec3 bitangent = cross(normal, tangent);

  gl_Position = u_projMatrix * u_viewMatrix * worldSpaceMatrix * vec4(vertex.position.xyz, 1.0);
  vertOut.fNormal = normal;
  vertOut.fTexCoords = vertex.texCoord.xy;
  vertOut.fTBN = mat3(tangent, bitangent, normal);
  vertOut.fMaskID = u_entityData[instance].u_maskID.xy;
  vertOut.fMaterialData = u_entityData[instance].u_material;
}

#type fragment
layout(location = 0) out vec4 gNormal;
layout(location = 1) out vec4 gAlbedo;
layout(location = 2) out vec4 gMatProp;
layout(location = 3) out vec4 gIDMaskColour;

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

  gAlbedo = vec4(pow(albedo.rgb * albedoReflectance.rgb, vec3(u_nearFarGamma.z)), 1.0);
  gAlbedo.a = texture(specF0Map, fragIn.fTexCoords).r * albedoReflectance.a;
  gNormal.rg = encodeNormal(getNormal(normalMap, fragIn.fTBN, fragIn.fTexCoords));
  gNormal.ba = 1.0.xx;

  gMatProp.r = texture(metallicMap, fragIn.fTexCoords).r * mrae.r;
  gMatProp.g = texture(roughnessMap, fragIn.fTexCoords).r * mrae.g;
  gMatProp.b = texture(aOcclusionMap, fragIn.fTexCoords).r * mrae.b;
  gMatProp.a = mrae.a;

  gIDMaskColour = vec4(fragIn.fMaskID.xxx, fragIn.fMaskID.y);
}
