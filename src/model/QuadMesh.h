#pragma once
#ifndef SSS_MODEL_QUADMODEL_H
#define SSS_MODEL_QUADMODEL_H

#include "BaseModel.h"
#include "Mesh.h"

#include <glad/glad.h>

namespace sss {

struct QuadVertex {
  Vec2f position;
  Vec2f uv;

  static void initBindings(GLuint va);
  static void cleanupBindings(GLuint va);
};

class QuadMesh : public Mesh<QuadVertex> {
public:
  void init();

private:
  using Mesh::init;
};

} // namespace sss

#endif
