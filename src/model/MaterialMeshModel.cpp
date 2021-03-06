#include "MaterialMeshModel.h"
#include "../utils/Image.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace sss {

bool MaterialMeshModel::load(const std::string& name, const Path& path) {
  m_name = name;
  std::cout << "Loading model \"" << this->name() << "\" from \"" << path << "\"" << std::endl;
  m_baseDir = path.dir();

  Assimp::Importer importer;
  // Importer options.
  // See http://assimp.sourceforge.net/lib_html/postprocess_8h.html.
  constexpr unsigned int flags = aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs;

  const aiScene* scene = importer.ReadFile(path.cstr(), flags);
  if (!scene) {
    std::cout << "Failed to load scene: " << importer.GetErrorString() << std::endl;
    return false;
  }

  m_meshes.reserve(scene->mNumMeshes);
  for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    loadMesh(scene->mMeshes[i], scene);
  m_meshes.shrink_to_fit();

  // Separate opaque/transparent objects.
  std::partition(m_meshes.begin(), m_meshes.end(),
                 [](const MaterialMesh& mesh) { return mesh.m_material.isOpaque; });

  std::cout << "Done:\n"
            << "> " << m_meshes.size() << " mesh(es)\n"
            << "> " << m_nbTriangles << " triangles\n"
            << "> " << m_nbVertices << " vertices" << std::endl;
  return true;
}

void MaterialMeshModel::render(const ShaderProgram& program) const {
  for (const MaterialMesh& m : m_meshes)
    m.render(program);
}

void MaterialMeshModel::renderForGBuf(const ShaderProgram& program) const {
  for (const MaterialMesh& m : m_meshes)
    m.renderForGBuf(program);
}

void MaterialMeshModel::bindMeshAlbedo(size_t meshIndex, GLuint binding) const {
  m_meshes[meshIndex].bindAlbedo(binding);
}

void MaterialMeshModel::release() {
  for (MaterialMesh& m : m_meshes)
    m.release();
  for (const Texture& t : m_loadedTextures)
    glDeleteTextures(1, &t.id);
  m_loadedTextures.clear();
}

void MaterialMeshModel::loadMesh(const aiMesh* mesh, const aiScene* scene) {
  const std::string meshName = name() + "_" + std::string(mesh->mName.C_Str());

  std::vector<MaterialMeshVertex> vertices;
  vertices.resize(mesh->mNumVertices);
  for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
    MaterialMeshVertex& vertex = vertices[v];

    vertex.position.x = mesh->mVertices[v].x;
    vertex.position.y = mesh->mVertices[v].y;
    vertex.position.z = mesh->mVertices[v].z;
    vertex.normal.x = mesh->mNormals[v].x;
    vertex.normal.y = mesh->mNormals[v].y;
    vertex.normal.z = mesh->mNormals[v].z;

    if (mesh->HasTextureCoords(0)) {
      vertex.texCoords.x = mesh->mTextureCoords[0][v].x;
      vertex.texCoords.y = mesh->mTextureCoords[0][v].y;

      vertex.tangent.x = mesh->mTangents[v].x;
      vertex.tangent.y = mesh->mTangents[v].y;
      vertex.tangent.z = mesh->mTangents[v].z;
      vertex.biTangent.x = mesh->mBitangents[v].x;
      vertex.biTangent.y = mesh->mBitangents[v].y;
      vertex.biTangent.z = mesh->mBitangents[v].z;
    } else {
      vertex.texCoords.x = 0.f;
      vertex.texCoords.y = 0.f;
    }
  }

  std::vector<unsigned int> indices;
  indices.resize(mesh->mNumFaces * 3);
  for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
    const aiFace& face = mesh->mFaces[f];
    const unsigned int f3 = f * 3;
    indices[f3] = face.mIndices[0];
    indices[f3 + 1] = face.mIndices[1];
    indices[f3 + 2] = face.mIndices[2];
  }

  const aiMaterial* mtl = scene->mMaterials[mesh->mMaterialIndex];
  Material material;
  if (!mtl) {
    std::cout << "No material assigned to mesh \"" << meshName << "\"\n, using default material"
              << std::endl;
  } else {
    material = loadMaterial(mtl);
  }

  m_nbTriangles += mesh->mNumFaces;
  m_nbVertices += mesh->mNumVertices;

  m_meshes.emplace_back();
  m_meshes.back().init(meshName, vertices, indices, material);
}

Material MaterialMeshModel::loadMaterial(const aiMaterial* mtl) {
  Material material;

  aiColor3D color;
  aiString texturePath;
  Texture texture;

  if (mtl->GetTextureCount(aiTextureType_AMBIENT) > 0) {
    mtl->GetTexture(aiTextureType_AMBIENT, 0, &texturePath);
    texture = loadTexture(texturePath, "ambient");
    if (texture.id != GL_INVALID_INDEX) {
      material.ambientMap = texture;
      material.hasAmbientMap = true;
    }
  } else if (mtl->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
    material.ambient = Vec3f(color.r, color.g, color.b);
  }

  if (mtl->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
    mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
    texture = loadTexture(texturePath, "diffuse");
    if (texture.id != GL_INVALID_INDEX) {
      material.diffuseMap = texture;
      material.hasDiffuseMap = true;
    }
  } else if (mtl->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
    material.diffuse = Vec3f(color.r, color.g, color.b);
  }

  if (mtl->GetTextureCount(aiTextureType_SPECULAR) > 0) {
    mtl->GetTexture(aiTextureType_SPECULAR, 0, &texturePath);
    texture = loadTexture(texturePath, "specular");
    if (texture.id != GL_INVALID_INDEX) {
      material.specularMap = texture;
      material.hasSpecularMap = true;
    }
  } else if (mtl->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
    material.specular = Vec3f(color.r, color.g, color.b);
  }

  float shininess;
  if (mtl->GetTextureCount(aiTextureType_SHININESS) > 0) {
    mtl->GetTexture(aiTextureType_SHININESS, 0, &texturePath);
    texture = loadTexture(texturePath, "shininess");
    if (texture.id != GL_INVALID_INDEX) {
      material.shininessMap = texture;
      material.hasShininessMap = true;
    }
  } else if (mtl->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
    material.shininess = shininess;
  }

  float normal;
  if (mtl->GetTextureCount(aiTextureType_NORMALS) > 0) {
    mtl->GetTexture(aiTextureType_NORMALS, 0, &texturePath);
    texture = loadTexture(texturePath, "normal");
    if (texture.id != GL_INVALID_INDEX) {
      material.normalMap = texture;
      material.hasNormalMap = true;
    }
  } else if (mtl->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normal) == AI_SUCCESS) {
    material.normal = normal;
  }

  if (mtl->GetTextureCount(aiTextureType_OPACITY) > 0)
    material.isOpaque = false;

  return material;
}

Texture MaterialMeshModel::loadTexture(const aiString& path, const std::string& type) {
  const char* path_str = path.C_Str();

  // Check if the texture has already been loaded.
  for (const Texture& t : m_loadedTextures) {
    if (t.path == path_str) {
      if (t.type == type) {
        return t;
      } else {
        // One texture can be used for more than one type.
        Texture texture;
        texture.id = t.id;
        texture.path = path_str;
        texture.type = type;
        return texture;
      }
    }
  }

  Texture texture;
  RGBImage image;
  const std::string fullPath = m_baseDir.str() + "/" + path_str;
  if (image.load(fullPath)) {
    glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);
    texture.path = path_str;
    texture.type = type;

    GLenum format = GL_INVALID_ENUM;
    GLenum internalFormat = GL_INVALID_ENUM;
    if (image.nbChannels() == 1) {
      format = GL_RED;
      internalFormat = GL_R32F;
    } else if (image.nbChannels() == 2) {
      format = GL_RG;
      internalFormat = GL_RG32F;
    } else if (image.nbChannels() == 3) {
      format = GL_RGB;
      internalFormat = GL_RGB32F;
    } else {
      format = GL_RGBA;
      internalFormat = GL_RGBA32F;
    }

    // Deduce the number of mipmaps.
    int w = image.width();
    int h = image.height();
    int mips = (int)glm::log2((float)glm::max(w, h));
    glTextureStorage2D(texture.id, mips, internalFormat, w, h);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTextureSubImage2D(texture.id, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, image.pixels());
    glGenerateTextureMipmap(texture.id);
  }

  m_loadedTextures.emplace_back(texture);
  return texture;
}

} // namespace sss
