#pragma once
#ifndef SSS_MODEL_BASEMODEL_H
#define SSS_MODEL_BASEMODEL_H

#include "../MathDefines.h"
#include "../shader/ShaderProgram.h"

namespace sss {

class BaseModel {
public:
  virtual ~BaseModel() = default;

  const Mat4f& transform() const { return m_transform; }
  void setTransform(const Mat4f& m) { m_transform = m; }

  virtual void render(const ShaderProgram& program) const = 0;
  virtual void renderForGBuf(const ShaderProgram& program) const {};

private:
  Mat4f m_transform = Mat4fId;
};

} // namespace sss

#endif
