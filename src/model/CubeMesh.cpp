#include "CubeMesh.h"

namespace sss {

void CubeVertex::initBindings(GLuint va) {
  glEnableVertexArrayAttrib(va, 0);

  glVertexArrayAttribBinding(va, 0, 0);
  glVertexArrayAttribFormat(va, 0, 3, GL_FLOAT, GL_FALSE, offsetof(CubeVertex, position));
}

void CubeVertex::cleanupBindings(GLuint va) { glDisableVertexArrayAttrib(va, 0); }

void CubeMesh::init() {
  std::vector<CubeVertex> vertices = {
    // clang-format off
    {{-0.5f, -0.5f, -0.5f}},
    {{ 0.5f, -0.5f, -0.5f}},
    {{ 0.5f,  0.5f, -0.5f}},
    {{ 0.5f,  0.5f, -0.5f}},
    {{-0.5f,  0.5f, -0.5f}},
    {{-0.5f, -0.5f, -0.5f}},

    {{-0.5f, -0.5f,  0.5f}},
    {{ 0.5f, -0.5f,  0.5f}},
    {{ 0.5f,  0.5f,  0.5f}},
    {{ 0.5f,  0.5f,  0.5f}},
    {{-0.5f,  0.5f,  0.5f}},
    {{-0.5f, -0.5f,  0.5f}},

    {{-0.5f,  0.5f,  0.5f}},
    {{-0.5f,  0.5f, -0.5f}},
    {{-0.5f, -0.5f, -0.5f}},
    {{-0.5f, -0.5f, -0.5f}},
    {{-0.5f, -0.5f,  0.5f}},
    {{-0.5f,  0.5f,  0.5f}},

    {{ 0.5f,  0.5f,  0.5f}},
    {{ 0.5f,  0.5f, -0.5f}},
    {{ 0.5f, -0.5f, -0.5f}},
    {{ 0.5f, -0.5f, -0.5f}},
    {{ 0.5f, -0.5f,  0.5f}},
    {{ 0.5f,  0.5f,  0.5f}},

    {{-0.5f, -0.5f, -0.5f}},
    {{ 0.5f, -0.5f, -0.5f}},
    {{ 0.5f, -0.5f,  0.5f}},
    {{ 0.5f, -0.5f,  0.5f}},
    {{-0.5f, -0.5f,  0.5f}},
    {{-0.5f, -0.5f, -0.5f}},

    {{-0.5f,  0.5f, -0.5f}},
    {{ 0.5f,  0.5f, -0.5f}},
    {{ 0.5f,  0.5f,  0.5f}},
    {{ 0.5f,  0.5f,  0.5f}},
    {{-0.5f,  0.5f,  0.5f}},
    {{-0.5f,  0.5f, -0.5f}}
    // clang-format on
  };

  init(vertices);
}

} // namespace sss
