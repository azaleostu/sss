#pragma once
#ifndef SSS_SHADER_SHADER_H
#define SSS_SHADER_SHADER_H

#include <glad/glad.h>
#include <string>

namespace sss {

class Shader {
public:
  Shader() = delete;
  Shader(std::string name, std::string shaderContentStr, GLenum type);

  ~Shader() { release(); }

  bool compile() const;
  void release();

  GLuint id() const { return m_id; }
  const std::string& name() const { return m_name; }

  void attachToProgram(GLuint id) const;
  void detachFromProgram(GLuint id) const;

private:
  GLuint m_id;
  std::string m_name;
  std::string m_source;
};

} // namespace sss

#endif
