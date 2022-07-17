#pragma once
#ifndef SSS_MODEL_CUBEMESH_H
#define SSS_MODEL_CUBEMESH_H

#include "BaseModel.h"
#include "Mesh.h"

#include <glad/glad.h>

namespace sss {

struct CubeVertex {
  Vec3f position;

  static void initBindings(GLuint va);
  static void cleanupBindings(GLuint va);
};

class CubeMesh : public Mesh<CubeVertex> {
public:
  void init();

private:
  using Mesh::init;
};

} // namespace sss

#endif
