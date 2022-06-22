#pragma once
#ifndef SSS_SHADER_SHADERPROGRAM_H
#define SSS_SHADER_SHADERPROGRAM_H

#include "Shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <utility>
#include <vector>
#include <array>

namespace sss {

class ShaderProgram {
public:
  ShaderProgram() = delete;
  explicit ShaderProgram(std::string dir)
    : m_dir(std::move(dir)) {}

  ~ShaderProgram() { release(); }

  GLuint id() const { return m_id; }
  const std::string& dir() const { return m_dir; }

  bool addShader(const std::string& name, GLenum type, const std::string& file);
  Shader* getShader(const char* name);

  void init();
  bool link();
  void release();

  void use() const;

public:
  GLint getUniformLocation(const char* name) const;

  void setBool(GLint loc, bool value) const;
  void setInt(GLint loc, int value) const;
  void setFloat(GLint loc, float value) const;
  void setVec2(GLint loc, const glm::vec2& value) const;
  void setVec2(GLint loc, float x, float y) const;
  void setVec3(GLint loc, const glm::vec3& value) const;
  void setVec3(GLint loc, float x, float y, float z) const;
  void setVec4(GLint loc, const glm::vec4& value) const;
  void setVec4(GLint loc, float x, float y, float z, float w) const;
  void setMat2(GLint loc, const glm::mat2& mat) const;
  void setMat3(GLint loc, const glm::mat3& mat) const;
  void setMat4(GLint loc, const glm::mat4& mat) const;
  void setVec4Array(const std::string& uniformName, const std::vector<glm::vec4>& mat) const;

private:
  GLuint m_id = GL_INVALID_INDEX;
  std::vector<Shader> m_shaders;
  std::string m_dir;
};

} // namespace sss

#endif
