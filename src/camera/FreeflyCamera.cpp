#include "FreeflyCamera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

void FreeflyCamera::setScreenSize(const int p_width, const int p_height) {
  _screenWidth = p_width;
  _screenHeight = p_height;
  _aspectRatio = float(_screenWidth) / _screenHeight;
  _updateVectors();
  _computeViewMatrix();
  _computeProjectionMatrix();
}

void FreeflyCamera::moveFront(const float p_delta) {
  _position -= _invDirection * p_delta;
  _computeViewMatrix();
}

void FreeflyCamera::moveRight(const float p_delta) {
  _position += _right * p_delta;
  _computeViewMatrix();
}

void FreeflyCamera::moveUp(const float p_delta) {
  _position += _up * p_delta;
  _computeViewMatrix();
}

void FreeflyCamera::rotate(const float p_yaw, const float p_pitch) {
  _yaw = glm::mod(_yaw + p_yaw, 360.f);
  _pitch = glm::clamp(_pitch + p_pitch, -89.f, 89.f);
  _updateVectors();
}

void FreeflyCamera::print() const {
  std::cout << "======== Camera ========" << std::endl;
  std::cout << "Position: " << glm::to_string(_position) << std::endl;
  std::cout << "View direction: " << glm::to_string(-_invDirection)
            << std::endl;
  std::cout << "Right: " << glm::to_string(_right) << std::endl;
  std::cout << "Up: " << glm::to_string(_up) << std::endl;
  std::cout << "Yaw: " << _yaw << std::endl;
  std::cout << "Pitch: " << _pitch << std::endl;
  std::cout << "========================" << std::endl;
}

Vec3f FreeflyCamera::getPosition() const { return _position; }

void FreeflyCamera::setPosition(const Vec3f& p_position) {
  _position = p_position;
  _computeViewMatrix();
}

void FreeflyCamera::setLookAt(const Vec3f& p_lookAt) {
  _invDirection = p_lookAt + _position;
  _computeViewMatrix();
}

void FreeflyCamera::setFovy(const float p_fovy) {
  _fovy = p_fovy;
  _computeProjectionMatrix();
}

void FreeflyCamera::_computeViewMatrix() {
  _viewMatrix = glm::lookAt(_position, _position - _invDirection, _up);
}

void FreeflyCamera::_computeProjectionMatrix() {
  _projectionMatrix =
      glm::perspective(glm::radians(_fovy), _aspectRatio, _zNear, _zFar);
}

void FreeflyCamera::_computeUMVPMatrix(const Mat4f& modelMatrix) {
  _uMVPMatrix = _projectionMatrix * _viewMatrix * modelMatrix;
}

void FreeflyCamera::_computeUMVMatrix(const Mat4f& modelMatrix) {
  _uMVMatrix = _viewMatrix * modelMatrix;
}

void FreeflyCamera::update() {
  _computeProjectionMatrix();
  _computeViewMatrix();
}

void FreeflyCamera::_updateVectors() {
  const float yaw = glm::radians(_yaw);
  const float pitch = glm::radians(_pitch);
  _invDirection =
      glm::normalize(Vec3f(glm::cos(yaw) * glm::cos(pitch), glm::sin(pitch),
                           glm::sin(yaw) * glm::cos(pitch)));
  _right = glm::normalize(
      glm::cross(Vec3f(0.f, 1.f, 0.f), _invDirection)); // We suppose 'y' as up.
  _up = glm::normalize(glm::cross(_invDirection, _right));

  _computeViewMatrix();
}
