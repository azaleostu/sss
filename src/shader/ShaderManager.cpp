#include "ShaderManager.h"
#include "../utils/ReadFile.h"

#include <glm/gtc/type_ptr.hpp>

namespace sss {

bool ShaderManager::addShader(const std::string& name, GLenum type, const std::string& file) {
  std::string source;
  if (!sss::readFile(m_dir + file, source))
    return false;

  m_shaders.emplace_back(name, source, type);
  m_shaders.back().attachToProgram(m_program);
  return true;
}

Shader* ShaderManager::getShader(const std::string& name) {
  for (Shader& s : m_shaders) {
    if (s.name() == name)
      return &s;
  }
  std::cout << "Could not find shader with name \"" << name << "\"" << std::endl;
  return nullptr;
}

void ShaderManager::use(GLuint& outId) const {
  glUseProgram(m_program);
  outId = m_program;
}

bool ShaderManager::link() const {
  glLinkProgram(m_program);
  GLint linked;
  glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLchar log[1024];
    glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
    std::cerr << "Failed to link program:\n" << log << std::endl;
    return false;
  }
  return true;
}

void ShaderManager::release() {
  glDeleteProgram(m_program);
  m_program = 0;
}

void ShaderManager::setBool(const char* name, bool value) const {
  glUniform1i(glGetUniformLocation(m_program, name), (int)value);
}

void ShaderManager::setInt(const char* name, int value) const {
  glUniform1i(glGetUniformLocation(m_program, name), value);
}

void ShaderManager::setFloat(const char* name, float value) const {
  glUniform1f(glGetUniformLocation(m_program, name), value);
}

void ShaderManager::setVec2(const char* name, const glm::vec2& value) const {
  glUniform2fv(glGetUniformLocation(m_program, name), 1, glm::value_ptr(value));
}
void ShaderManager::setVec2(const char* name, float x, float y) const {
  glUniform2f(glGetUniformLocation(m_program, name), x, y);
}

void ShaderManager::setVec3(const char* name, const glm::vec3& value) const {
  glUniform3fv(glGetUniformLocation(m_program, name), 1, glm::value_ptr(value));
}
void ShaderManager::setVec3(const char* name, float x, float y, float z) const {
  glUniform3f(glGetUniformLocation(m_program, name), x, y, z);
}

void ShaderManager::setVec4(const char* name, const glm::vec4& value) const {
  glUniform4fv(glGetUniformLocation(m_program, name), 1, glm::value_ptr(value));
}
void ShaderManager::setVec4(const char* name, float x, float y, float z, float w) const {
  glUniform4f(glGetUniformLocation(m_program, name), x, y, z, w);
}

void ShaderManager::setMat2(const char* name, const glm::mat2& mat) const {
  glUniformMatrix2fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderManager::setMat3(const char* name, const glm::mat3& mat) const {
  glUniformMatrix3fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderManager::setMat4(const char* name, const glm::mat4& mat) const {
  glUniformMatrix4fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(mat));
}

} // namespace sss
