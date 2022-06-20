#pragma once
#ifndef SSS_SHADER_SHADER_H
#define SSS_SHADER_SHADER_H

#include <glad/glad.h>
#include <string>

namespace sss {

class Shader {
public:
  Shader() = delete;
  Shader(const char* sourceStr, GLenum type);

  ~Shader() { release(); }

  bool compile();
  void release();

  GLuint id() const { return m_id; }

private:
  GLuint m_id;
};

} // namespace sss

#endif
