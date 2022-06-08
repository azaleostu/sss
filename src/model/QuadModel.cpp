#include "QuadModel.h"
#include "../MathDefines.h"

namespace sss {

void QuadModel::init() {
  glCreateVertexArrays(1, &m_VA);
  glCreateBuffers(1, &m_VB);
  glCreateBuffers(1, &m_IB);

  struct QuadVertex {
    Vec2f position;
    Vec2f uv;
  };
  QuadVertex vertices[] = {
    // clang-format off
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{-1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
    // clang-format on
  };
  glNamedBufferData(m_VB, sizeof(vertices), vertices, GL_STATIC_DRAW);

  unsigned short indices[]{
    // clang-format off
    0, 1, 2,
    1, 3, 2,
    // clang-format on
  };
  glNamedBufferData(m_IB, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexArrayVertexBuffer(m_VA, 0, m_VB, 0, sizeof(QuadVertex));
  glVertexArrayElementBuffer(m_VA, m_IB);

  glVertexArrayAttribBinding(m_VA, 0, 0);
  glVertexArrayAttribBinding(m_VA, 1, 0);
  glEnableVertexArrayAttrib(m_VA, 0);
  glEnableVertexArrayAttrib(m_VA, 1);
  glVertexArrayAttribFormat(m_VA, 0, 2, GL_FLOAT, GL_FALSE, offsetof(QuadVertex, position));
  glVertexArrayAttribFormat(m_VA, 1, 2, GL_FLOAT, GL_FALSE, offsetof(QuadVertex, uv));
}

void QuadModel::release() {
  glDisableVertexArrayAttrib(m_VA, 0);
  glDisableVertexArrayAttrib(m_VA, 1);

  glDeleteBuffers(1, &m_IB);
  glDeleteBuffers(1, &m_VB);
  glDeleteVertexArrays(1, &m_VA);
}

void QuadModel::render(const ShaderProgram& program) const {
  program.use();
  glBindVertexArray(m_VA);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
  glBindVertexArray(0);
}

} // namespace sss
