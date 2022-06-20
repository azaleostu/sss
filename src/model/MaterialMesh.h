#pragma once
#ifndef SSS_MODEL_TRIANGLEMESH_H
#define SSS_MODEL_TRIANGLEMESH_H

#include "Mesh.h"

#include <glad/glad.h>
#include <vector>

namespace sss {

struct MaterialMeshVertex {
  Vec3f position;
  Vec3f normal;
  Vec2f texCoords;
  Vec3f tangent;
  Vec3f biTangent;

  static void initBindings(GLuint va);
  static void cleanupBindings(GLuint va);
};

struct Texture {
  unsigned int id;
  std::string type;
  std::string path;
};

struct Material {
  Vec3f ambient = Vec3fZero;
  Vec3f diffuse = Vec3fZero;
  Vec3f specular = Vec3fZero;
  float shininess = 0.f;
  float normal = 0.f;

  bool hasAmbientMap = false;
  bool hasDiffuseMap = false;
  bool hasSpecularMap = false;
  bool hasShininessMap = false;
  bool hasNormalMap = false;

  Texture ambientMap;
  Texture diffuseMap;
  Texture specularMap;
  Texture shininessMap;
  Texture normalMap;

  bool isOpaque = true;
  bool isLiquid = false;
};

class MaterialMesh : public Mesh<MaterialMeshVertex> {
  friend class MaterialMeshModel;

public:
  const std::string& name() const { return m_name; }

  void init(const std::string& name, const std::vector<MaterialMeshVertex>& vertices,
            const std::vector<unsigned int>& indices, const Material& material);

  void render(const ShaderProgram& program) const override;

private:
  using Mesh::init;

  void loadUniforms(const ShaderProgram& program) const;

private:
  Material m_material;
  std::string m_name;
};

} // namespace sss

#endif
