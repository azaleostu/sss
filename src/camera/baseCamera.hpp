#ifndef __BASE_CAMERA_HPP__
#define __BASE_CAMERA_HPP__

#include "../define.hpp"

class BaseCamera {
protected:
  BaseCamera() {}

public:
  inline const Mat4f& getViewMatrix() const { return _viewMatrix; }
  inline const Mat4f& getProjectionMatrix() const { return _projectionMatrix; }
  inline const Mat4f& getUMVPMatrix() const { return _uMVPMatrix; }
  inline const Mat4f& getUMVMatrix() const { return _uMVMatrix; }
  Vec3f getPosition() const { return _position; }
  inline const float& getFovy() const { return _fovy; }

  virtual void setPosition(const Vec3f& p_position) = 0;
  virtual void setLookAt(const Vec3f& p_lookAt) = 0;
  virtual void setFovy(const float p_fovy) = 0;

  virtual void setScreenSize(const int p_width, const int p_height) = 0;

  virtual void moveFront(const float p_delta) = 0;
  virtual void moveRight(const float p_delta) = 0;
  virtual void moveUp(const float p_delta) = 0;
  virtual void rotate(const float p_yaw, const float p_pitch) = 0;

  virtual void print() const = 0;

  virtual void _computeUMVPMatrix(const Mat4f& modelMatrix) = 0;
  virtual void _computeUMVMatrix(const Mat4f& modelMatrix) = 0;

  virtual void update() = 0;

protected:
  virtual void _computeViewMatrix() = 0;
  virtual void _computeProjectionMatrix() = 0;
  virtual void _updateVectors() = 0;

protected:
  Vec3f _position = VEC3F_ZERO;
  Vec3f _invDirection = Vec3f(0.f, 0.f, 1.f); // Dw dans le cours.
  Vec3f _right = Vec3f(-1.f, 0.f, 0.f);       // Rw dans le cours.
  Vec3f _up = Vec3f(0.f, 1.f, 0.f);           // Uw dans le cours.
  // Angles defining the orientation in degrees
  float _yaw = 90.f;
  float _pitch = 0.f;

  int _screenWidth = 1280;
  int _screenHeight = 720;
  float _aspectRatio = float(_screenWidth) / _screenHeight;
  float _fovy = 60.f;
  float _zNear = 0.1f;
  float _zFar = 5000.f;

  Mat4f _viewMatrix = MAT4F_ID;
  Mat4f _projectionMatrix = MAT4F_ID;
  Mat4f _uMVPMatrix = MAT4F_ID;
  Mat4f _uMVMatrix = MAT4F_ID;
};

#endif // __BASE_CAMERA_HPP__
