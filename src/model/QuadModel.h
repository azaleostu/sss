#pragma once
#ifndef SSS_MODEL_QUADMODEL_H
#define SSS_MODEL_QUADMODEL_H

#include "../shader/ShaderProgram.h"

#include <glad/glad.h>

namespace sss {

class QuadModel {
public:
  ~QuadModel() { release(); }

  void init();
  void release();

  void render(const ShaderProgram& program) const;

private:
  GLuint m_VA = 0;
  GLuint m_VB = 0;
  GLuint m_IB = 0;
};

} // namespace sss

#endif
