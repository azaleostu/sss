#include "MaterialMesh.h"

namespace sss {

void MaterialMeshVertex::initBindings(GLuint va) {
  glEnableVertexArrayAttrib(va, 0);
  glEnableVertexArrayAttrib(va, 1);
  glEnableVertexArrayAttrib(va, 2);
  glEnableVertexArrayAttrib(va, 3);
  glEnableVertexArrayAttrib(va, 4);

  glVertexArrayAttribBinding(va, 0, 0);
  glVertexArrayAttribBinding(va, 1, 0);
  glVertexArrayAttribBinding(va, 2, 0);
  glVertexArrayAttribBinding(va, 3, 0);
  glVertexArrayAttribBinding(va, 4, 0);

  glVertexArrayAttribFormat(va, 0, 3, GL_FLOAT, GL_FALSE, offsetof(MaterialMeshVertex, position));
  glVertexArrayAttribFormat(va, 1, 3, GL_FLOAT, GL_FALSE, offsetof(MaterialMeshVertex, normal));
  glVertexArrayAttribFormat(va, 2, 2, GL_FLOAT, GL_FALSE, offsetof(MaterialMeshVertex, texCoords));
  glVertexArrayAttribFormat(va, 3, 3, GL_FLOAT, GL_FALSE, offsetof(MaterialMeshVertex, tangent));
  glVertexArrayAttribFormat(va, 4, 3, GL_FLOAT, GL_FALSE, offsetof(MaterialMeshVertex, biTangent));
}

void MaterialMeshVertex::cleanupBindings(GLuint va) {
  glDisableVertexArrayAttrib(va, 0);
  glDisableVertexArrayAttrib(va, 1);
  glDisableVertexArrayAttrib(va, 2);
  glDisableVertexArrayAttrib(va, 3);
  glDisableVertexArrayAttrib(va, 4);
}

void MaterialMesh::init(const std::string& name, const std::vector<MaterialMeshVertex>& vertices,
                        const std::vector<unsigned int>& indices, const Material& material) {
  m_material = material;
  m_name = name;
  Mesh::init(vertices, indices);
}

void MaterialMesh::render(const ShaderProgram& program) const {
  loadUniforms(program);

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

  Mesh::render(program);

  glBindTextureUnit(1, 0);
  glBindTextureUnit(2, 0);
  glBindTextureUnit(3, 0);
  glBindTextureUnit(4, 0);
  glBindTextureUnit(5, 0);
}

void MaterialMesh::renderForGBuf(const ShaderProgram& program) const {
  loadGBufUniforms(program);

  if (m_material.hasDiffuseMap)
    glBindTextureUnit(1, m_material.diffuseMap.id);
  if (m_material.hasSpecularMap)
    glBindTextureUnit(2, m_material.specularMap.id);
  if (m_material.hasNormalMap)
    glBindTextureUnit(3, m_material.normalMap.id);

  Mesh::render(program);

  glBindTextureUnit(1, 0);
  glBindTextureUnit(2, 0);
  glBindTextureUnit(3, 0);
}

void MaterialMesh::loadUniforms(const ShaderProgram& program) const {
  program.setVec3(program.getUniformLocation("uAmbient"), m_material.ambient);
  program.setVec3(program.getUniformLocation("uDiffuse"), m_material.diffuse);
  program.setVec3(program.getUniformLocation("uSpecular"), m_material.specular);
  program.setFloat(program.getUniformLocation("uShininess"), m_material.shininess);

  program.setBool(program.getUniformLocation("uHasDiffuseMap"), m_material.hasDiffuseMap);
  program.setBool(program.getUniformLocation("uHasAmbientMap"), m_material.hasAmbientMap);
  program.setBool(program.getUniformLocation("uHasSpecularMap"), m_material.hasSpecularMap);
  program.setBool(program.getUniformLocation("uHasShininessMap"), m_material.hasShininessMap);
  program.setBool(program.getUniformLocation("uHasNormalMap"), m_material.hasNormalMap);

  program.setBool(program.getUniformLocation("uIsOpaque"), m_material.isOpaque);
  program.setBool(program.getUniformLocation("uIsLiquid"), m_material.isLiquid);
}

void MaterialMesh::loadGBufUniforms(const ShaderProgram& program) const {
  program.setBool(program.getUniformLocation("uHasAlbedoTex"), m_material.hasDiffuseMap);
  program.setBool(program.getUniformLocation("uHasSpecularTex"), m_material.hasSpecularMap);
  program.setBool(program.getUniformLocation("uHasNormalMap"), m_material.hasNormalMap);

  program.setVec3(program.getUniformLocation("uFallbackAlbedo"), m_material.diffuse);
  program.setVec3(program.getUniformLocation("uFallbackSpecular"), m_material.specular);
}

} // namespace sss
