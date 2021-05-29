#include "Graphics/Model.h"

// Project includes.
#include "Core/Logs.h"

namespace SciRenderer
{
  Model::Model()
    : loaded(false)
  { }

  Model::~Model()
  { }

  void
  Model::loadModel(const std::string &filepath)
  {
    Logger* logs = Logger::getInstance();

    auto flags = aiProcess_CalcTangentSpace | aiProcess_GenNormals
               | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate
               | aiProcess_GenUVCoords | aiProcess_SortByPType;

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filepath, flags);
    if (!scene)
    {
      logs->logMessage(LogMessage("Model failed to load at the path " + filepath +
                                  ", with the error: " + importer.GetErrorString()
                                  + ".", true, false, true));
      return;
    }
    else if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      logs->logMessage(LogMessage("Model failed to load at the path " + filepath +
                                  ", with the error: " + importer.GetErrorString()
                                  + ".", true, false, true));
      return;
    }

    this->filepath = filepath;

    this->processNode(scene->mRootNode, scene);
  }

  // Recursively process all the nodes in the mesh.
  void
  Model::processNode(aiNode* node, const aiScene* scene)
  {
    for (GLuint i = 0; i < node->mNumMeshes; i++)
    {
      aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      this->processMesh(mesh, scene);
    }

    for (GLuint i = 0; i < node->mNumChildren; i++)
      this->processNode(node->mChildren[i], scene);
  }

  // Process each individual mesh.
  void
  Model::processMesh(aiMesh* mesh, const aiScene* scene)
  {
    std::vector<Vertex> meshVertices;
    std::vector<GLuint> meshIndicies;
    std::vector<Texture2D> meshTextures;

    // Get the positions.
    if (mesh->HasPositions())
    {
      meshVertices.reserve(mesh->mNumVertices);

      for (GLuint i = 0; i < mesh->mNumVertices; i++)
        meshVertices.push_back(Vertex());

      glm::vec4 temp;
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        temp.x = mesh->mVertices[i].x;
        temp.y = mesh->mVertices[i].y;
        temp.z = mesh->mVertices[i].z;
        temp.w = 1.0f;

        meshVertices[i].position = temp;
      }
    }
    else
    {
      // Nothing that can be done for this mesh as it has no data.
      return;
    }

    if (mesh->HasNormals())
    {
      glm::vec3 temp;
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
      {
        temp.x = mesh->mNormals[i].x;
        temp.y = mesh->mNormals[i].y;
        temp.z = mesh->mNormals[i].z;

        meshVertices[i].normal = temp;
      }
    }

    // Get the colours, but only supporting a single colour channel for now.
    {
      glm::vec3 temp = glm::vec3(1.0f);
      for (GLuint i = 0; i < mesh->mNumVertices; i++)
        meshVertices[i].colour = temp;
    }


    // Get the UV's, but only supporting a single UV channel for now.
    if (mesh->GetNumUVChannels() > 0)
    {
      if (mesh->HasTextureCoords(0))
      {
        // Loop over the texture coords and assign them.
        glm::vec2 temp;
        for (GLuint i = 0; i < mesh->mNumUVComponents[0]; i++)
        {
          temp.x = mesh->mTextureCoords[0][i].x;
          temp.y = mesh->mTextureCoords[0][i].y;

          meshVertices[i].uv = temp;
        }

        // Just in case not all the vertices had texture coords, loop over the
        // rest and assign 0.0f.
        for (GLuint i = mesh->mNumUVComponents[0]; i < mesh->mNumVertices; i++)
        {
          temp.x = 0.0f;
          temp.y = 0.0f;

          meshVertices[i].uv = temp;
        }
      }
    }

    // Fetch the tangents and bitangents.
    if (mesh->HasTangentsAndBitangents())
    {
      glm::vec3 temp1, temp2;
      for (GLuint i = 0; i <mesh->mNumVertices; i++)
      {
        temp1.x = mesh->mTangents[i].x;
        temp1.y = mesh->mTangents[i].y;
        temp1.z = mesh->mTangents[i].z;

        temp2.x = mesh->mBitangents[i].x;
        temp2.y = mesh->mBitangents[i].y;
        temp2.z = mesh->mBitangents[i].z;

        meshVertices[i].tangent = temp1;
        meshVertices[i].bitangent = temp2;
      }
    }

    // Fetch the indicies.
    for (GLuint i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];

      for (GLuint j = 0; j < face.mNumIndices; j++)
        meshIndicies.push_back(face.mIndices[j]);
    }

    this->subMeshes.push_back(createShared<Mesh>(meshVertices, meshIndicies, meshTextures, this));

    this->loaded = true;
  }
}
