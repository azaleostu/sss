#include "QuadMesh.h"

namespace sss {

void QuadVertex::initBindings(GLuint va) {
  glEnableVertexArrayAttrib(va, 0);
  glEnableVertexArrayAttrib(va, 1);

  glVertexArrayAttribBinding(va, 0, 0);
  glVertexArrayAttribBinding(va, 1, 0);
  glVertexArrayAttribFormat(va, 0, 2, GL_FLOAT, GL_FALSE, offsetof(QuadVertex, position));
  glVertexArrayAttribFormat(va, 1, 2, GL_FLOAT, GL_FALSE, offsetof(QuadVertex, uv));
}

void QuadVertex::cleanupBindings(GLuint va) {
  glDisableVertexArrayAttrib(va, 0);
  glDisableVertexArrayAttrib(va, 1);
}

void QuadMesh::init() {
  std::vector<QuadVertex> vertices = {
    // clang-format off
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{-1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
    // clang-format on
  };

  std::vector<GLuint> indices = {
    // clang-format off
    0, 1, 2,
    1, 3, 2,
    // clang-format on
  };

  init(vertices, indices);
}

} // namespace sss
