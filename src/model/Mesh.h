#pragma once
#ifndef SSS_MODEL_MESH_H
#define SSS_MODEL_MESH_H

#include "BaseModel.h"

#include <glad/glad.h>
#include <vector>

namespace sss {

template <typename Vertex> class Mesh : public BaseModel {
public:
  ~Mesh() override { release(); }

  void init(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
  void release();

  void render(const ShaderProgram& program) const override;

private:
  size_t m_numIndices = 0;
  GLuint m_VA = 0;
  GLuint m_VB = 0;
  GLuint m_IB = 0;
};

} // namespace sss

#include "Mesh.inl"

#endif
