#pragma once
#ifndef SSS_MODELS_BASEMODEL_H
#define SSS_MODELS_BASEMODEL_H

#include "../MathDefines.h"
#include "../shader/ShaderProgram.h"

#include <glad/glad.h>
#include <string>
#include <utility>

namespace sss {

class BaseModel {
public:
  BaseModel() = default;
  explicit BaseModel(std::string name)
    : m_name(std::move(name)) {}

  virtual ~BaseModel() = default;

  virtual void render(const ShaderProgram& program) const = 0;
  virtual void cleanGL() = 0;

  const Mat4f& transformation() const { return m_transformation; }
  const std::string& name() const { return m_name; }

  void setTransformation(const Mat4f& t) { m_transformation = t; }
  void setName(std::string name) { m_name = std::move(name); }

private:
  Mat4f m_transformation = Mat4fId;
  std::string m_name = "none";
};

} // namespace sss

#endif
