#pragma once
#ifndef SSS_MODELS_TRIANGLEMESHMODEL_H
#define SSS_MODELS_TRIANGLEMESHMODEL_H

#include "../utils/Path.h"
#include "BaseModel.h"
#include "TriangleMesh.h"

#include <assimp/scene.h>

namespace sss {

class TriangleMeshModel : public BaseModel {
public:
  // Load a 3D model with Assimp.
  bool load(const std::string& name, const Path& path);

  void render(GLuint program) const override;
  void update();
  void cleanGL() override;

private:
  void loadMesh(const aiMesh* mesh, const aiScene* scene);
  Material loadMaterial(const aiMaterial* mtl);
  Texture loadTexture(const aiString& path, const std::string& type);

public:
  Path m_baseDir = "";

  std::vector<TriangleMesh> m_meshes;
  std::vector<Texture> m_loadedTextures;

  unsigned int m_nbTriangles = 0;
  unsigned int m_nbVertices = 0;
};

} // namespace sss

#endif
