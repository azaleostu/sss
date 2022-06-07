#include "Shader.h"

#include <utility>

// Precompiled:
// iostream

namespace sss {

Shader::Shader(std::string name, const char* sourceStr, GLenum type)
  : m_id(glCreateShader(type))
  , m_name(std::move(name)) {
  glShaderSource(m_id, 1, &sourceStr, nullptr);
}

bool Shader::compile() {
  glCompileShader(m_id);

  GLint compiled;
  glGetShaderiv(m_id, GL_COMPILE_STATUS, &compiled);
  if (compiled)
    return true;

  GLint infoLogLength;
  glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
  auto* infoLog = new GLchar[infoLogLength];
  glGetShaderInfoLog(m_id, infoLogLength, nullptr, infoLog);
  std::cout << "Failed to compile shader \"" << m_name << "\":" << infoLog << std::endl;
  release();
  delete[] infoLog;
  return false;
}

void Shader::release() {
  glDeleteShader(m_id);
  m_id = 0;
}

} // namespace sss
