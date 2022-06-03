#ifndef __TRIANGLE_MESH_HPP__
#define __TRIANGLE_MESH_HPP__

#include "../define.hpp"
#include <glad/glad.h>
#include <iostream>
#include <vector>

struct Vertex {
  Vec3f _position;
  Vec3f _normal;
  Vec2f _texCoords;
  Vec3f _tangent;
  Vec3f _bitangent;
};

struct Texture {
  unsigned int _id;
  std::string _type;
  std::string _path;
};

struct Material {
  Vec3f _ambient = VEC3F_ZERO;
  Vec3f _diffuse = VEC3F_ZERO;
  Vec3f _specular = VEC3F_ZERO;
  float _shininess = 0.f;
  float _normal = 0.f;

  bool _hasAmbientMap = false;
  bool _hasDiffuseMap = false;
  bool _hasSpecularMap = false;
  bool _hasShininessMap = false;
  bool _hasNormalMap = false;

  Texture _ambientMap;
  Texture _diffuseMap;
  Texture _specularMap;
  Texture _shininessMap;
  Texture _normalMap;

  bool _opaque = true;

  bool _liquid = false;
};

class TriangleMesh {
public:
  TriangleMesh() = delete;
  TriangleMesh(const std::string& p_name, const std::vector<Vertex>& p_vertices,
               const std::vector<unsigned int>& p_indices,
               const Material& p_material);

  ~TriangleMesh() = default;

  void render(const GLuint p_glProgram) const;

  void update();

  void cleanGL();

private:
  void _setupGL();

public:
  std::string _name = "Unknown";

  // ================ Geometric data.
  std::vector<Vertex> _vertices;
  std::vector<unsigned int> _indices;

  // ================ Material data.
  Material _material;

  // ================ GL data.
  GLuint _vao = GL_INVALID_INDEX; // Vertex Array Object
  GLuint _vbo = GL_INVALID_INDEX; // Vertex Buffer Object
  GLuint _ebo = GL_INVALID_INDEX; // Element Buffer Object
};

#endif // __TRIANGLE_MESH_HPP__
