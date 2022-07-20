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

long double eumelaninAbsorbtion(const float lambda) {
  return 6.6 * powl(10.0, 11.0)  * powl(lambda, -3.33);
}

long double pheomelaninAbsorbtion(const float lambda) {
  return 2.9 * powl(10.0, 15.0) * powl(lambda, -4.75);
}

long double skinBaseAbsorption(const float lambda) {
  return 7.84 * powl(10.0, 8.0) * powl(lambda, -3.255);
}

// spectral absorption of epidermis
long double specAbsE(const float lambda // wavelength
                     ,
                     const float Vm // melanin volume fraction
                     ,
                     const float PhiM // melanin types ratio
) {
  long double MuA_eu = eumelaninAbsorbtion(lambda);
  long double MuA_ph = pheomelaninAbsorbtion(lambda);
  long double MuA_betaCaro = 0.0;
  long double MuA_base = skinBaseAbsorption(lambda);
  long double MuEpiderm =
    Vm * (PhiM * MuA_eu + (1.0 - PhiM) * MuA_ph) + (1.0 - Vm) * (MuA_betaCaro + MuA_base);
  return MuEpiderm;
}

long double oxyHaemoglobinAbsorption(const float lambda) {
  long double Whb = 64500.0; // Molar weight of Haemoglobin
  long double Phb = 150.0; // haemoglobin Concentration
  long double Epsi_hbo2 = 818.0; // Oxy-Haemoglobin Extinction
  return 2.303 * ((Phb * Epsi_hbo2) / Whb);
}

long double deoxyHaemoglobinAbsorption(const float lambda) {
  long double Whb = 64500.0;     // Molar weight of Haemoglobin
  long double Phb = 150.0;       // haemoglobin Concentration
  long double Epsi_hb = 818.0; // Deoxy-Haemoglobin Extinction
  return 2.303 * ((Phb * Epsi_hb) / Whb);
}

// spectral absorption of dermis
long double specAbsD(const float lambda // wavelength
                     ,
                     const float Vb // blood volume fraction
                     ,
                     const float PhiH // haemoglobin types ratio
) {
  long double MuA_hb = deoxyHaemoglobinAbsorption(lambda);
  long double MuA_hbo2 = deoxyHaemoglobinAbsorption(lambda);
  long double MuDerm = oxyHaemoglobinAbsorption(lambda);
  long double MuA_base = skinBaseAbsorption(lambda);
  long double MuA_bil = 0.0;
  long double MuA_betaCaro = 0.0;
    Vb * (PhiH * MuA_hb + (1.0 - PhiH) * MuA_hbo2 + MuA_bil + MuA_betaCaro) + (1.0 - Vb) * MuA_base;
  return MuDerm;
}

} // namespace sss

#endif
