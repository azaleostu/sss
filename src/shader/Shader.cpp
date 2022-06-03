#include "Shader.h"

#include <utility>

// Precompiled:
// iostream

namespace sss {

Shader::Shader(std::string name, std::string shaderContentStr, GLenum type)
  : m_id(glCreateShader(type))
  , m_name(std::move(name))
  , m_source(std::move(shaderContentStr)) {
  const char* source_str = m_source.c_str();
  glShaderSource(m_id, 1, &source_str, nullptr);
}

bool Shader::compile() const {
  glCompileShader(m_id);
  GLint compiled;
  glGetShaderiv(m_id, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLchar log[1024];
    glGetShaderInfoLog(m_id, sizeof(log), nullptr, log);
    glDeleteShader(m_id);
    std::cout << "Failed to compile shader:\n" << log << std::endl;
    return false;
  }
  return true;
}

void Shader::release() {
  glDeleteShader(m_id);
  m_id = 0;
}

void Shader::attachToProgram(GLuint id) const { glAttachShader(id, m_id); }
void Shader::detachFromProgram(GLuint id) const { glDetachShader(id, m_id); }

} // namespace sss
