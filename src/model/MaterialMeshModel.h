#pragma once
#ifndef SSS_MODELS_TRIANGLEMESHMODEL_H
#define SSS_MODELS_TRIANGLEMESHMODEL_H

#include "../utils/Path.h"
#include "BaseModel.h"
#include "MaterialMesh.h"

#include <assimp/scene.h>

namespace sss {

class MaterialMeshModel : public BaseModel {
public:
  ~MaterialMeshModel() override { release(); }

  const Path& baseDir() const { return m_baseDir; }
  const std::string& name() const { return m_name; }

  // Load a 3D model with Assimp.
  bool load(const std::string& name, const Path& path);

  void render(const ShaderProgram& program) const override;
  void release();

private:
  void loadMesh(const aiMesh* mesh, const aiScene* scene);
  Material loadMaterial(const aiMaterial* mtl);
  Texture loadTexture(const aiString& path, const std::string& type);

private:
  Path m_baseDir = "";
  std::string m_name = "none";

  std::vector<MaterialMesh> m_meshes;
  std::vector<Texture> m_loadedTextures;

  unsigned int m_nbTriangles = 0;
  unsigned int m_nbVertices = 0;
};

} // namespace sss

#endif
