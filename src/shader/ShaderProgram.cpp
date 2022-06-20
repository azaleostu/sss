#include "ShaderProgram.h"
#include "../utils/ReadFile.h"

#include <glm/gtc/type_ptr.hpp>

namespace sss {

void ShaderProgram::init() { m_id = glCreateProgram(); }

bool ShaderProgram::addShader(GLenum type, const std::string& file) {
  std::string source;
  if (!sss::readFile(s_shadersDir + file, source))
    return false;

  m_shaders.emplace_back(source.c_str(), type);
  if (!m_shaders.back().compile()) {
    m_shaders.pop_back();
    return false;
  }
  glAttachShader(m_id, m_shaders.back().id());
  return true;
}

bool ShaderProgram::link() {
  glLinkProgram(m_id);

  GLint linked;
  glGetProgramiv(m_id, GL_LINK_STATUS, &linked);
  if (linked)
    return true;

  GLint infoLogLength;
  glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
  auto* infoLog = new GLchar[infoLogLength];
  glGetProgramInfoLog(m_id, infoLogLength, nullptr, infoLog);
  std::cerr << "Failed to link program:\n" << infoLog << std::endl;
  release();
  delete[] infoLog;
  return false;
}

void ShaderProgram::release() {
  glDeleteProgram(m_id);
  m_id = 0;
}

bool ShaderProgram::initVertexFragment(const std::string& vertexPath,
                                       const std::string& fragmentPath) {
  init();
  if (!addShader(GL_VERTEX_SHADER, vertexPath) || !addShader(GL_FRAGMENT_SHADER, fragmentPath))
    return false;

  return link();
}

void ShaderProgram::use() const { glUseProgram(m_id); }

GLint ShaderProgram::getUniformLocation(const char* name) const {
  return glGetUniformLocation(m_id, name);
}

void ShaderProgram::setBool(GLint loc, bool value) const {
  glProgramUniform1i(m_id, loc, (int)value);
}

void ShaderProgram::setInt(GLint loc, int value) const { glProgramUniform1i(m_id, loc, value); }
void ShaderProgram::setFloat(GLint loc, float value) const { glProgramUniform1f(m_id, loc, value); }

void ShaderProgram::setVec2(GLint loc, const glm::vec2& value) const {
  glProgramUniform2fv(m_id, loc, 1, glm::value_ptr(value));
}
void ShaderProgram::setVec2(GLint loc, float x, float y) const {
  glProgramUniform2f(m_id, loc, x, y);
}

void ShaderProgram::setVec3(GLint loc, const glm::vec3& value) const {
  glProgramUniform3fv(m_id, loc, 1, glm::value_ptr(value));
}
void ShaderProgram::setVec3(GLint loc, float x, float y, float z) const {
  glProgramUniform3f(m_id, loc, x, y, z);
}

void ShaderProgram::setVec4(GLint loc, const glm::vec4& value) const {
  glProgramUniform4fv(m_id, loc, 1, glm::value_ptr(value));
}
void ShaderProgram::setVec4(GLint loc, float x, float y, float z, float w) const {
  glProgramUniform4f(m_id, loc, x, y, z, w);
}

void ShaderProgram::setMat2(GLint loc, const glm::mat2& mat) const {
  glProgramUniformMatrix2fv(m_id, loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderProgram::setMat3(GLint loc, const glm::mat3& mat) const {
  glProgramUniformMatrix3fv(m_id, loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderProgram::setMat4(GLint loc, const glm::mat4& mat) const {
  glProgramUniformMatrix4fv(m_id, loc, 1, GL_FALSE, glm::value_ptr(mat));
}

} // namespace sss
