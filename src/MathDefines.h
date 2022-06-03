#pragma once
#ifndef SSS_MATHDEFINES_H
#define SSS_MATHDEFINES_H

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <limits>

namespace sss {

constexpr float Pi = glm::pi<float>();
constexpr float Pi2 = glm::half_pi<float>();
constexpr float Pi4 = glm::quarter_pi<float>();
constexpr float Pi32 = glm::three_over_two_pi<float>();
constexpr float TwoPi = glm::two_pi<float>();
constexpr float InvPi = glm::one_over_pi<float>();
constexpr float Inv2Pi = glm::one_over_two_pi<float>();
constexpr float Inf = std::numeric_limits<float>::infinity();

using Vec2i = glm::ivec2;
using Vec3i = glm::ivec3;
using Vec4i = glm::ivec4;
using Vec2f = glm::vec2;
using Vec3f = glm::vec3;
using Vec4f = glm::vec4;

constexpr Vec2i Vec2iZero = Vec2i(0);
constexpr Vec3i Vec3iZero = Vec3i(0);
constexpr Vec4i Vec4iZero = Vec4i(0);
constexpr Vec2f Vec2fZero = Vec2f(0.0f);
constexpr Vec3f Vec3fZero = Vec3f(0.0f);
constexpr Vec4f Vec4fZero = Vec4f(0.0f);

using Mat3f = glm::mat3;
using Mat4f = glm::mat4;

constexpr Mat4f Mat3fId = Mat3f(1.0f);
constexpr Mat4f Mat4fId = Mat4f(1.0f);

} // namespace sss

#endif
