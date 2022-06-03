#ifndef SHADER_MANAGER
#define SHADER_MANAGER

#include "Shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <utility>
#include <vector>

namespace sss {

class ShaderManager {
public:
  ShaderManager() = delete;
  explicit ShaderManager(std::string dir)
    : m_dir(std::move(dir)) {}

  ~ShaderManager() { release(); }

  bool addShader(const std::string& name, GLenum type, const std::string& file);
  Shader* getShader(const std::string& name);

  void init() { m_program = glCreateProgram(); }
  bool link() const;
  void release();

  void use(GLuint& outId) const;

public:
  void setBool(const char* name, bool value) const;
  void setInt(const char* name, int value) const;
  void setFloat(const char* name, float value) const;
  void setVec2(const char* name, const glm::vec2& value) const;
  void setVec2(const char* name, float x, float y) const;
  void setVec3(const char* name, const glm::vec3& value) const;
  void setVec3(const char* name, float x, float y, float z) const;
  void setVec4(const char* name, const glm::vec4& value) const;
  void setVec4(const char* name, float x, float y, float z, float w) const;
  void setMat2(const char* name, const glm::mat2& mat) const;
  void setMat3(const char* name, const glm::mat3& mat) const;
  void setMat4(const char* name, const glm::mat4& mat) const;

private:
  std::vector<Shader> m_shaders;
  std::string m_dir;
  GLuint m_program = GL_INVALID_INDEX;
};

} // namespace sss

#endif
