#pragma once
#ifndef SSS_CAMERA_BASECAMERA_H
#define SSS_CAMERA_BASECAMERA_H

#include "../MathDefines.h"

namespace sss {

class BaseCamera {
protected:
  BaseCamera() = default;

public:
  virtual ~BaseCamera() = default;

  Vec3f position() const { return m_position; }
  float fovy() const { return m_fovy; }
  const Mat4f& viewMatrix() const { return m_viewMatrix; }
  const Mat4f& projectionMatrix() const { return m_projectionMatrix; }
  const Mat4f& MVMatrix() const { return m_MVMatrix; }
  const Mat4f& MVPMatrix() const { return m_MVPMatrix; }

  void setSpeed(float speed) { m_speed = speed; }

  virtual void setPosition(const Vec3f& position) = 0;
  virtual void setFovy(float fovy) = 0;
  virtual void setLookAt(const Vec3f& lookAt) = 0;
  virtual void setScreenSize(int width, int height) = 0;

  virtual void moveFront() = 0;
  virtual void moveBack() = 0;
  virtual void moveRight() = 0;
  virtual void moveLeft() = 0;
  virtual void moveUp() = 0;
  virtual void moveDown() = 0;
  virtual void rotate(float yaw, float pitch) = 0;

  virtual void print() const = 0;
  virtual void update() = 0;

  virtual void computeMVMatrix(const Mat4f& modelMatrix) = 0;
  virtual void computeMVPMatrix(const Mat4f& modelMatrix) = 0;

protected:
  virtual void computeViewMatrix() = 0;
  virtual void computeProjectionMatrix() = 0;
  virtual void updateVectors() = 0;

protected:
  Vec3f m_position = Vec3fZero;
  Vec3f m_invDirection = Vec3f(0.0f, 0.0f, 1.0f);
  Vec3f m_right = Vec3f(-1.0f, 0.0f, 0.0f);
  Vec3f m_up = Vec3f(0.0f, 1.0f, 0.0f);
  float m_speed = 1.0f;

  float m_yaw = 90.0f;
  float m_pitch = 0.0f;
  float m_rotationSensitivity = 0.1f;

  int m_screenWidth = 1280;
  int m_screenHeight = 720;
  float m_aspectRatio = (float)m_screenWidth / (float)m_screenHeight;
  float m_fovy = 60.f;
  float m_zNear = 0.1f;
  float m_zFar = 5000.f;

  Mat4f m_viewMatrix = Mat4fId;
  Mat4f m_projectionMatrix = Mat4fId;
  Mat4f m_MVMatrix = Mat4fId;
  Mat4f m_MVPMatrix = Mat4fId;
};

} // namespace sss

#endif
