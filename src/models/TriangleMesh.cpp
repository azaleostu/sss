#include "TriangleMesh.h"

#include <glm/gtc/type_ptr.hpp>
#include <utility>

// Precompiled:
// iostream

namespace sss {

TriangleMesh::TriangleMesh(std::string name, std::vector<Vertex> vertices,
                           std::vector<unsigned int> indices, Material material)
  : m_name(std::move(name))
  , m_vertices(std::move(vertices))
  , m_indices(std::move(indices))
  , m_material(std::move(material)) {
  m_vertices.shrink_to_fit();
  m_indices.shrink_to_fit();
  setupGL();
}

void TriangleMesh::update() {}

void TriangleMesh::render(const GLuint program) const {
  glUseProgram(program);

  glProgramUniform3fv(program, glGetUniformLocation(program, "uAmbient"), 1,
                      glm::value_ptr(m_material.ambient));
  glProgramUniform3fv(program, glGetUniformLocation(program, "uDiffuse"), 1,
                      glm::value_ptr(m_material.diffuse));
  glProgramUniform3fv(program, glGetUniformLocation(program, "uSpecular"), 1,
                      glm::value_ptr(m_material.specular));
  glProgramUniform1f(program, glGetUniformLocation(program, "uShininess"), m_material.shininess);
  glProgramUniform1i(program, glGetUniformLocation(program, "uHasDiffuseMap"),
                     m_material.hasDiffuseMap);
  glProgramUniform1i(program, glGetUniformLocation(program, "uHasAmbientMap"),
                     m_material.hasAmbientMap);
  glProgramUniform1i(program, glGetUniformLocation(program, "uHasSpecularMap"),
                     m_material.hasSpecularMap);
  glProgramUniform1i(program, glGetUniformLocation(program, "uHasShininessMap"),
                     m_material.hasShininessMap);
  glProgramUniform1i(program, glGetUniformLocation(program, "uHasNormalMap"),
                     m_material.hasNormalMap);
  glProgramUniform1i(program, glGetUniformLocation(program, "uIsOpaque"), m_material.isOpaque);
  glProgramUniform1i(program, glGetUniformLocation(program, "uIsLiquid"), m_material.isLiquid);

  if (m_material.hasDiffuseMap)
    glBindTextureUnit(1, m_material.diffuseMap.id);
  if (m_material.hasAmbientMap)
    glBindTextureUnit(2, m_material.ambientMap.id);
  if (m_material.hasSpecularMap)
    glBindTextureUnit(3, m_material.specularMap.id);
  if (m_material.hasShininessMap)
    glBindTextureUnit(4, m_material.shininessMap.id);
  if (m_material.hasNormalMap)
    glBindTextureUnit(5, m_material.normalMap.id);

  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, (GLsizei)m_indices.size(), GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);

  glBindTextureUnit(1, 0);
  glBindTextureUnit(2, 0);
  glBindTextureUnit(3, 0);
  glBindTextureUnit(4, 0);
  glBindTextureUnit(5, 0);
}

void TriangleMesh::cleanGL() {
  glDisableVertexArrayAttrib(m_VAO, 0);
  glDisableVertexArrayAttrib(m_VAO, 1);
  glDisableVertexArrayAttrib(m_VAO, 2);
  glDisableVertexArrayAttrib(m_VAO, 3);
  glDisableVertexArrayAttrib(m_VAO, 4);
  glDeleteVertexArrays(1, &m_VAO);
  glDeleteBuffers(1, &m_VBO);
  glDeleteBuffers(1, &m_EBO);
}

void TriangleMesh::setupGL() {
  glCreateVertexArrays(1, &m_VAO);
  glCreateBuffers(1, &m_VBO);
  glCreateBuffers(1, &m_EBO);

  glNamedBufferData(m_VBO, (GLsizei)(m_vertices.size() * sizeof(Vertex)), m_vertices.data(),
                    GL_STATIC_DRAW);
  glNamedBufferData(m_EBO, (GLsizei)(m_indices.size() * sizeof(unsigned int)), m_indices.data(),
                    GL_STATIC_DRAW);

  glVertexArrayVertexBuffer(m_VAO, 0, m_VBO, 0, sizeof(Vertex));
  glVertexArrayElementBuffer(m_VAO, m_EBO);

  glVertexArrayAttribBinding(m_VAO, 0, 0);
  glVertexArrayAttribBinding(m_VAO, 1, 0);
  glVertexArrayAttribBinding(m_VAO, 2, 0);
  glVertexArrayAttribBinding(m_VAO, 3, 0);
  glVertexArrayAttribBinding(m_VAO, 4, 0);

  glEnableVertexArrayAttrib(m_VAO, 0);
  glEnableVertexArrayAttrib(m_VAO, 1);
  glEnableVertexArrayAttrib(m_VAO, 2);
  glEnableVertexArrayAttrib(m_VAO, 3);
  glEnableVertexArrayAttrib(m_VAO, 4);

  glVertexArrayAttribFormat(m_VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
  glVertexArrayAttribFormat(m_VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
  glVertexArrayAttribFormat(m_VAO, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoords));
  glVertexArrayAttribFormat(m_VAO, 3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
  glVertexArrayAttribFormat(m_VAO, 4, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, biTangent));
}

} // namespace sss
