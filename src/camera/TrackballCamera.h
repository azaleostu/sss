#pragma once
#ifndef SSS_TRACKBALL_CAMERA_H
#define SSS_TRACKBALL_CAMERA_H

#include "../MathDefines.h"
#include "BaseCamera.h"

namespace sss {

class TrackballCamera : public BaseCamera {
public:
  float zoomSpeed = 0.025f;
  float moveSpeed = 0.5f;

  void updatePosition();
  void setPosition(const Vec3f& position) override;
  void setLookAt(const Vec3f& lookAt) override;
  void setFovy(float fovy) override;
  void setScreenSize(int width, int height) override;

  void setSubjectDistance(float distance);

  void moveFront() override;
  void moveBack() override;
  void moveLeft() override;
  void moveRight() override;
  void moveUp() override;
  void moveDown() override;
  void rotate(float yaw, float pitch) override;

  void print() const override;
  void update() override;

  void processKeyEvent(const SDL_Event& e) override;
  void processMouseEvent(const SDL_Event& e) override;

private:
  void computeViewMatrix() override;
  void computeProjectionMatrix() override;
  void updateVectors() override;

private:
  Vec3f m_subjectPosition = Vec3f(0.0f, 0.0f, 0.0f);
  float m_subjectDistance = 0.5f;
};

} // namespace sss

#endif // SSS_TRACKBALL_CAMERA_H
