#include "ShaderManager.hpp"
#include "../utils/readFile.hpp"

ShaderManager::~ShaderManager() {
  shaders.clear();
  glDeleteProgram(program);
}

void ShaderManager::addShader(const std::string& name, const GLenum& type,
                              const std::string& file) {
  shaders.emplace_back(name, readFile(folder + file), type);
  shaders.back().attachToProgram(program);
}

shader* ShaderManager::getShader(const std::string& name) {
  for (size_t i = 0; i < shaders.size(); i++) {
    if (!shaders[i].name.compare(name))
      return &shaders[i];
  }
  std::cout << "ERROR : No shader found named " << name << std::endl;
  return nullptr;
}

void ShaderManager::use(GLuint& prgrm) {
  glUseProgram(program);
  prgrm = program;
}

void ShaderManager::link() {
  glLinkProgram(program);
  // Check if link is ok.
  GLint linked;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLchar log[1024];
    glGetProgramInfoLog(program, sizeof(log), NULL, log);
    std::cerr << "Error linking program: " << log << std::endl;
    exit(1);
  }
}

void ShaderManager::setBool(const std::string& name, bool value) const {
  glUniform1i(glGetUniformLocation(program, name.c_str()), (int)value);
}

void ShaderManager::setInt(const std::string& name, int value) const {
  glUniform1i(glGetUniformLocation(program, name.c_str()), value);
}

void ShaderManager::setFloat(const std::string& name, float value) const {
  glUniform1f(glGetUniformLocation(program, name.c_str()), value);
}

void ShaderManager::setVec2(const std::string& name,
                            const glm::vec2& value) const {
  glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]);
}
void ShaderManager::setVec2(const std::string& name, float x, float y) const {
  glUniform2f(glGetUniformLocation(program, name.c_str()), x, y);
}

void ShaderManager::setVec3(const std::string& name,
                            const glm::vec3& value) const {
  glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]);
}
void ShaderManager::setVec3(const std::string& name, float x, float y,
                            float z) const {
  glUniform3f(glGetUniformLocation(program, name.c_str()), x, y, z);
}

void ShaderManager::setVec4(const std::string& name,
                            const glm::vec4& value) const {
  glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]);
}
void ShaderManager::setVec4(const std::string& name, float x, float y, float z,
                            float w) {
  glUniform4f(glGetUniformLocation(program, name.c_str()), x, y, z, w);
}

void ShaderManager::setMat2(const std::string& name,
                            const glm::mat2& mat) const {
  glUniformMatrix2fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE,
                     &mat[0][0]);
}

void ShaderManager::setMat3(const std::string& name,
                            const glm::mat3& mat) const {
  glUniformMatrix3fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE,
                     &mat[0][0]);
}

void ShaderManager::setMat4(const std::string& name,
                            const glm::mat4& mat) const {
  glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE,
                     &mat[0][0]);
}
