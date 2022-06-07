#pragma once
#ifndef SSS_MODEL_TRIANGLEMESH_H
#define SSS_MODEL_TRIANGLEMESH_H

#include "../MathDefines.h"
#include "../shader/ShaderProgram.h"

#include <glad/glad.h>
#include <iostream>
#include <vector>

namespace sss {

struct Vertex {
  Vec3f position;
  Vec3f normal;
  Vec2f texCoords;
  Vec3f tangent;
  Vec3f biTangent;
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

class TriangleMesh {
  friend class TriangleMeshModel;

public:
  TriangleMesh() = delete;
  TriangleMesh(std::string name, std::vector<Vertex> vertices, std::vector<unsigned int> indices,
               Material material);

  void render(const ShaderProgram& program) const;
  void cleanGL();

private:
  void setupGL();

private:
  std::string m_name = "none";

  std::vector<Vertex> m_vertices;
  std::vector<unsigned int> m_indices;

  Material m_material;

  GLuint m_VAO = GL_INVALID_INDEX;
  GLuint m_VBO = GL_INVALID_INDEX;
  GLuint m_EBO = GL_INVALID_INDEX;
};

} // namespace sss

#endif
