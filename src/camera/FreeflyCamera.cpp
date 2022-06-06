#include "FreeflyCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// Precompiled:
// iostream

namespace sss {

void FreeflyCamera::setPosition(const Vec3f& position) {
  m_position = position;
  computeViewMatrix();
}

void FreeflyCamera::setFovy(float fovy) {
  m_fovy = fovy;
  computeProjectionMatrix();
}

void FreeflyCamera::setLookAt(const Vec3f& lookAt) {
  m_invDirection = lookAt + m_position;
  computeViewMatrix();
}

void FreeflyCamera::setScreenSize(int width, int height) {
  m_screenWidth = width;
  m_screenHeight = height;
  m_aspectRatio = (float)m_screenWidth / (float)m_screenHeight;
  updateVectors();
  computeViewMatrix();
  computeProjectionMatrix();
}

void FreeflyCamera::moveFront() {
  m_position -= m_invDirection * m_speed;
  computeViewMatrix();
}

void FreeflyCamera::moveBack() {
  m_position += m_invDirection * m_speed;
  computeViewMatrix();
}

void FreeflyCamera::moveRight() {
  m_position += m_right * m_speed;
  computeViewMatrix();
}

void FreeflyCamera::moveLeft() {
  m_position -= m_right * m_speed;
  computeViewMatrix();
}

void FreeflyCamera::moveUp() {
  m_position += m_up * m_speed;
  computeViewMatrix();
}

void FreeflyCamera::moveDown() {
  m_position -= m_up * m_speed;
  computeViewMatrix();
}

void FreeflyCamera::rotate(float yaw, float pitch) {
  m_yaw = glm::mod(m_yaw + yaw * m_rotationSensitivity, 360.f);
  m_pitch = glm::clamp(m_pitch + pitch * m_rotationSensitivity, -89.f, 89.f);
  updateVectors();
}

void FreeflyCamera::print() const {
  std::cout << "======== Camera ========" << std::endl;
  std::cout << "Position: " << glm::to_string(m_position) << std::endl;
  std::cout << "View direction: " << glm::to_string(-m_invDirection) << std::endl;
  std::cout << "Right: " << glm::to_string(m_right) << std::endl;
  std::cout << "Up: " << glm::to_string(m_up) << std::endl;
  std::cout << "Yaw: " << m_yaw << std::endl;
  std::cout << "Pitch: " << m_pitch << std::endl;
  std::cout << "========================" << std::endl;
}

void FreeflyCamera::update() {
  computeProjectionMatrix();
  computeViewMatrix();
}

void FreeflyCamera::computeViewMatrix() {
  m_viewMatrix = glm::lookAt(m_position, m_position - m_invDirection, m_up);
}

void FreeflyCamera::computeProjectionMatrix() {
  m_projectionMatrix = glm::perspective(glm::radians(m_fovy), m_aspectRatio, m_zNear, m_zFar);
}

void FreeflyCamera::updateVectors() {
  const float yaw = glm::radians(m_yaw);
  const float pitch = glm::radians(m_pitch);
  m_invDirection = glm::normalize(
    Vec3f(glm::cos(yaw) * glm::cos(pitch), glm::sin(pitch), glm::sin(yaw) * glm::cos(pitch)));

  constexpr Vec3f Up = Vec3f(0.0f, 1.0f, 0.0f);
  m_right = glm::normalize(glm::cross(Up, m_invDirection));
  m_up = glm::normalize(glm::cross(m_invDirection, m_right));

  computeViewMatrix();
}

} // namespace sss
