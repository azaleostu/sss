#pragma once
#ifndef SSS_MODELS_BASEMODEL_H
#define SSS_MODELS_BASEMODEL_H

#include "../MathDefines.h"

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

  virtual void render(GLuint program) const = 0;
  virtual void cleanGL() = 0;

public:
  Mat4f m_transformation = Mat4fId;
  std::string m_name = "none";
};

} // namespace sss

#endif
