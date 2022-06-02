#ifndef __FREEFLY_CAMERA_HPP__
#define __FREEFLY_CAMERA_HPP__

#include "../define.hpp"
#include "baseCamera.hpp"

class FreeflyCamera : public BaseCamera {
public:
  FreeflyCamera() {}

  inline const Mat4f& getViewMatrix() const { return _viewMatrix; }
  inline const Mat4f& getProjectionMatrix() const { return _projectionMatrix; }
  inline const Mat4f& getUMVPMatrix() const { return _uMVPMatrix; }
  inline const Mat4f& getUMVMatrix() const { return _uMVMatrix; }
  Vec3f getPosition() const;

  void setPosition(const Vec3f& p_position);
  void setLookAt(const Vec3f& p_lookAt);
  void setFovy(const float p_fovy);

  void setScreenSize(const int p_width, const int p_height);

  void moveFront(const float p_delta);
  void moveRight(const float p_delta);
  void moveUp(const float p_delta);
  void rotate(const float p_yaw, const float p_pitch);

  void print() const;

  void _computeUMVPMatrix(const Mat4f& modelMatrix);
  void _computeUMVMatrix(const Mat4f& modelMatrix);

private:
  void _computeViewMatrix();
  void _computeProjectionMatrix();
  void update();
  void _updateVectors();
};

#endif // __FREEFLY_CAMERA_HPP__
