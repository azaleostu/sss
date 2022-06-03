#pragma once
#ifndef SSS_FREEFLY_CAMERA_H
#define SSS_FREEFLY_CAMERA_H

#include "../MathDefines.h"
#include "BaseCamera.h"

namespace sss {

class FreeflyCamera : public BaseCamera {
public:
  void setPosition(const Vec3f& position) override;
  void setLookAt(const Vec3f& lookAt) override;
  void setFovy(float fovy) override;
  void setScreenSize(int width, int height) override;

  void moveFront() override;
  void moveBack() override;
  void moveLeft() override;
  void moveRight() override;
  void moveUp() override;
  void moveDown() override;
  void rotate(float yaw, float pitch) override;

  void print() const override;
  void update() override;

  void computeMVMatrix(const Mat4f& modelMatrix) override;
  void computeMVPMatrix(const Mat4f& modelMatrix) override;

private:
  void computeViewMatrix() override;
  void computeProjectionMatrix() override;
  void updateVectors() override;
};

} // namespace sss

#endif
