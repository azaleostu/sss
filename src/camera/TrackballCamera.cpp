#include "TrackballCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// Precompiled:
// iostream

namespace sss {
void TrackballCamera::setScreenSize(int width, int height) {
  m_screenWidth = width;
  m_screenHeight = height;
  m_aspectRatio = (float)m_screenWidth / (float)m_screenHeight;
  updateVectors();
  computeViewMatrix();
  computeProjectionMatrix();
}

void TrackballCamera::moveFront() {
  _subjectPosition -= m_invDirection * m_speed;
  computeViewMatrix();
  updatePosition();
}

void TrackballCamera::moveBack() {
  _subjectPosition += m_invDirection * m_speed;
  computeViewMatrix();
  updatePosition();
}

void TrackballCamera::moveRight() {
  _subjectPosition += m_right * m_speed;
  computeViewMatrix();
  updatePosition();
}

void TrackballCamera::moveLeft() {
  _subjectPosition -= m_right * m_speed;
  computeViewMatrix();
  updatePosition();
}

void TrackballCamera::moveUp() {
  _subjectPosition += m_up * m_speed;
  computeViewMatrix();
  updatePosition();
}

void TrackballCamera::moveDown() {
  _subjectPosition -= m_up * m_speed;
  computeViewMatrix();
  updatePosition();
}

void TrackballCamera::rotate(const float p_yaw, const float p_pitch) {
  m_yaw = glm::mod(m_yaw + p_yaw, 360.f);
  m_pitch = glm::clamp(m_pitch + p_pitch, -89.f, 89.f);
  updateVectors();
  updatePosition();
}

void TrackballCamera::print() const {
  std::cout << "======== Camera ========" << std::endl;
  std::cout << "Camera position: " << glm::to_string(m_position) << std::endl;
  std::cout << "Subject position: " << glm::to_string(_subjectPosition) << std::endl;
  std::cout << "Subject distance: " << _subjectDistance << std::endl;
  std::cout << "View direction: " << glm::to_string(-m_invDirection) << std::endl;
  std::cout << "Right: " << glm::to_string(m_right) << std::endl;
  std::cout << "Up: " << glm::to_string(m_up) << std::endl;
  std::cout << "Yaw: " << m_yaw << std::endl;
  std::cout << "Pitch: " << m_pitch << std::endl;
  std::cout << "========================" << std::endl;
}

void TrackballCamera::updatePosition() {
  updateVectors();
  m_position = _subjectPosition + Vec3f(m_invDirection.x * _subjectDistance,
                                        m_invDirection.y * _subjectDistance,
                                        m_invDirection.z * _subjectDistance);
  computeViewMatrix();
}

void TrackballCamera::setPosition(const Vec3f& p_position) {
  _subjectPosition = p_position;
  computeViewMatrix();
  updatePosition();
}

void TrackballCamera::setLookAt(const Vec3f& lookAt) {
  m_invDirection = lookAt + m_position;
  computeViewMatrix();
}

void TrackballCamera::setFovy(const float fovy) {
  m_fovy = fovy;
  computeProjectionMatrix();
}

void TrackballCamera::computeViewMatrix() {
  m_viewMatrix = glm::lookAt(m_position, m_position - m_invDirection, m_up);
}

void TrackballCamera::computeProjectionMatrix() {
  m_projectionMatrix = glm::perspective(glm::radians(m_fovy), m_aspectRatio, m_zNear, m_zFar);
}

void TrackballCamera::update() {
  updatePosition();
  computeProjectionMatrix();
  computeViewMatrix();
}

void TrackballCamera::updateVectors() {
  const float yaw = glm::radians(m_yaw);
  const float pitch = glm::radians(m_pitch);
  m_invDirection = glm::normalize(
    Vec3f(glm::cos(yaw) * glm::cos(pitch), glm::sin(pitch), glm::sin(yaw) * glm::cos(pitch)));
  m_right =
    glm::normalize(glm::cross(Vec3f(0.f, 1.f, 0.f), m_invDirection)); // We suppose 'y' as up.
  m_up = glm::normalize(glm::cross(m_invDirection, m_right));
}

} // namespace sss
