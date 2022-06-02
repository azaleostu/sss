#ifndef __BASE_OBJECT_HPP__
#define __BASE_OBJECT_HPP__

#include <glad/glad.h>
#include "define.hpp"
#include <string>

class BaseModel {
public:
  BaseModel() = default;
  BaseModel(const std::string& p_name) : _name(p_name) {}

  virtual ~BaseModel() = default;

  virtual void render(const GLuint p_glProgram) const = 0;

  virtual void cleanGL() = 0;

public:
  std::string _name = "Unknown";
  Mat4f _transformation = MAT4F_ID;
};

#endif // __BASE_OBJECT_HPP__
