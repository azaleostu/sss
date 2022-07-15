#include "TrackballCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace sss {

void TrackballCamera::setScreenSize(int width, int height) {
  m_screenWidth = width;
  m_screenHeight = height;
  m_aspectRatio = (float)m_screenWidth / (float)m_screenHeight;
  updateVectors();
  computeViewMatrix();
  computeProjectionMatrix();
}

void TrackballCamera::setSubjectDistance(float distance) {
  m_subjectDistance = distance;
  updatePosition();
}

void TrackballCamera::moveFront() {
  m_subjectPosition -= m_invDirection * m_speed;
  updatePosition();
}

void TrackballCamera::moveBack() {
  m_subjectPosition += m_invDirection * m_speed;
  updatePosition();
}

void TrackballCamera::moveRight() {
  m_subjectPosition += m_right * m_speed;
  updatePosition();
}

void TrackballCamera::moveLeft() {
  m_subjectPosition -= m_right * m_speed;
  updatePosition();
}

void TrackballCamera::moveUp() {
  m_subjectPosition += m_up * m_speed;
  updatePosition();
}

void TrackballCamera::moveDown() {
  m_subjectPosition -= m_up * m_speed;
  updatePosition();
}

void TrackballCamera::rotate(const float p_yaw, const float p_pitch) {
  m_yaw = glm::mod(m_yaw + p_yaw, 360.f);
  m_pitch = glm::clamp(m_pitch + p_pitch, -89.f, 89.f);
  updatePosition();
}

void TrackballCamera::print() const {
  std::cout << "======== Camera ========" << std::endl;
  std::cout << "Camera position: " << glm::to_string(m_position) << std::endl;
  std::cout << "Subject position: " << glm::to_string(m_subjectPosition) << std::endl;
  std::cout << "Subject distance: " << m_subjectDistance << std::endl;
  std::cout << "Zoom speed: " << zoomSpeed << std::endl;
  std::cout << "View direction: " << glm::to_string(-m_invDirection) << std::endl;
  std::cout << "Right: " << glm::to_string(m_right) << std::endl;
  std::cout << "Up: " << glm::to_string(m_up) << std::endl;
  std::cout << "Yaw: " << m_yaw << std::endl;
  std::cout << "Pitch: " << m_pitch << std::endl;
  std::cout << "========================" << std::endl;
}

void TrackballCamera::updatePosition() {
  updateVectors();
  m_position = m_subjectPosition + m_subjectDistance * m_invDirection;
  computeViewMatrix();
}

void TrackballCamera::setPosition(const Vec3f& p_position) {
  m_subjectPosition = p_position;
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
}

void TrackballCamera::processKeyEvent(const SDL_Event& e) {
  if (e.type == SDL_KEYDOWN && (e.key.keysym.scancode == SDL_SCANCODE_SPACE))
    print();
}

void TrackballCamera::processMouseEvent(const SDL_Event& e) {
  if (e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON_LMASK)
    rotate((float)e.motion.xrel * moveSpeed, (float)e.motion.yrel * moveSpeed);

  if (e.type == SDL_MOUSEWHEEL) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_LSHIFT])
      setFovy(fovy() - (float)e.wheel.y);
    else
      setSubjectDistance(glm::max(m_subjectDistance - (float)e.wheel.y * zoomSpeed, 0.0f));
  }
}

void TrackballCamera::updateVectors() {
  const float yaw = glm::radians(m_yaw);
  const float pitch = glm::radians(m_pitch);
  m_invDirection = glm::normalize(
    Vec3f(glm::cos(yaw) * glm::cos(pitch), glm::sin(pitch), glm::sin(yaw) * glm::cos(pitch)));
  m_right = glm::cross(Vec3f(0.0f, 1.0f, 0.0f), m_invDirection);
  m_up = glm::cross(m_invDirection, m_right);
}

} // namespace sss
