#pragma once
#ifndef SSS_MODELS_TRIANGLEMESHMODEL_H
#define SSS_MODELS_TRIANGLEMESHMODEL_H

#include "../shader/ShaderProgram.h"
#include "../utils/Path.h"
#include "TriangleMesh.h"

#include <assimp/scene.h>

namespace sss {

class TriangleMeshModel {
public:
  ~TriangleMeshModel() { cleanGL(); }

  const Mat4f& transformation() const { return m_transformation; }
  const Path& baseDir() const { return m_baseDir; }
  const std::string& name() const { return m_name; }

  void setTransformation(const Mat4f& t) { m_transformation = t; }

  // Load a 3D model with Assimp.
  bool load(const std::string& name, const Path& path);

  void render(const ShaderProgram& program) const;
  void cleanGL();

private:
  void loadMesh(const aiMesh* mesh, const aiScene* scene);
  Material loadMaterial(const aiMaterial* mtl);
  Texture loadTexture(const aiString& path, const std::string& type);

private:
  Mat4f m_transformation = Mat4fId;
  Path m_baseDir = "";
  std::string m_name = "none";

  std::vector<TriangleMesh> m_meshes;
  std::vector<Texture> m_loadedTextures;

  unsigned int m_nbTriangles = 0;
  unsigned int m_nbVertices = 0;
};

} // namespace sss

#endif
