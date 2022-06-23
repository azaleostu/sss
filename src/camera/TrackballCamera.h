#pragma once
#ifndef SSS_TRACKBALL_CAMERA_H
#define SSS_TRACKBALL_CAMERA_H

#include "../MathDefines.h"
#include "BaseCamera.h"

namespace sss {

class TrackballCamera : public BaseCamera {
public:
  TrackballCamera() {}

  void updatePosition();
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

private:
  void computeViewMatrix() override;
  void computeProjectionMatrix() override;
  void updateVectors() override;

public:
  Vec3f _subjectPosition = Vec3f(0.f, 0.f, 0.f);
  float _subjectDistance = 0.5f;
};
} // namespace sss

#endif // SSS_TRACKBALL_CAMERA_H
