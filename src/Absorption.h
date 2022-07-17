#pragma once
#ifndef SSS_ABSORPTION_H
#define SSS_ABSORPTION_H

#include "MathDefines.h"

namespace sss {

float transmissionForWavelength(float wavelength) {
  const float x = wavelength;
  const float val =
    ((float)(-0.00000155 * (x * x * x)) + 0.0024713f * (x * x) - 1.01845f * x + 127.489f) / 100.f;
  return val < 0.f ? 0.f : val > 1.f ? 1.f : val; // result between 0 and 1
}

Vec3f absorb(const Vec3f& color, const float wavelength) {
  if (wavelength >= 380.f && wavelength < 410.f) {
    return color * Vec3f(0.6f - 0.41f * ((410.f - wavelength) / 30.f), 0.f,
                         0.39f + 0.6f * ((410.f - wavelength) / 30.f));
  } else if (wavelength >= 410.f && wavelength < 440.f) {
    return color * Vec3f(0.19f - 0.19f * ((440.f - wavelength) / 30.f), 0.f, 1.f);
  } else if (wavelength >= 440.f && wavelength < 490.f) {
    return color * Vec3f(0.f, 1.f - (490.f - wavelength) / 50.f, 1.f);
  } else if (wavelength >= 490.f && wavelength < 510.f) {
    return color * Vec3f(0.f, 1.f, (510.f - wavelength) / 20.f);
  } else if (wavelength >= 510.f && wavelength < 580.f) {
    return color * Vec3f(1.f - ((580.f - wavelength) / 70.f), 1.f, 0.f);
  } else if (wavelength >= 580.f && wavelength < 640.f) {
    return color * Vec3f(1.f, (640.f - wavelength) / 60.f, 0.f);
  } else if (wavelength >= 640 && wavelength < 700) {
    return color * Vec3f(1.f, 0.f, 0.f);
  } else if (wavelength >= 700 && wavelength < 780.f) {
    return color * Vec3f(0.35f - 0.65f * ((780.f - wavelength) / 80.f), 0.f, 0.f);
  } else {
    return color;
  }
}

} // namespace sss

#endif
