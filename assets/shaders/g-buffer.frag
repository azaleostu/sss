#version 460

in vec3 vNormal;
in vec3 vFragPos;
in vec2 vUV;
in mat3 vTBN;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec2 gUV;
layout(location = 2) out vec3 gNormal;
layout(location = 3) out vec3 gAlbedo;
layout(location = 4) out vec3 gIrradiance;

layout(binding = 0) uniform samplerCube uEnvIrradianceMap;
layout(binding = 1) uniform sampler2D uAlbedoMap;
layout(binding = 2) uniform sampler2D uNormalMap;

layout(binding = 3) uniform sampler2D uSkinParamTex;
layout(binding = 4) uniform sampler2D uSkinColorLookupTex;

layout(binding = 5) uniform sampler2D uParamTex;//0,0 is up left

uniform bool uHasNormalMap;

struct Light {
  vec3 position;
  vec3 direction;
  vec3 color;
  float intensity;
};

uniform Light uLight;

uniform bool uGammaCorrect;
uniform bool uUseDynamicSkinColor;
uniform bool uUseEnvIrradiance;

uniform float uB;
uniform float uS;
uniform float uF;
uniform float uW;
uniform float uM;

vec3 absorb(const vec3 color, const float wavelength) {
  if (wavelength >= 380.f && wavelength < 410.f) {
    return color * vec3(0.6f - 0.41f * ((410.f - wavelength) / 30.f), 0.f,
                         0.39f + 0.6f * ((410.f - wavelength) / 30.f));
  } else if (wavelength >= 410.f && wavelength < 440.f) {
    return color * vec3(0.19f - 0.19f * ((440.f - wavelength) / 30.f), 0.f, 1.f);
  } else if (wavelength >= 440.f && wavelength < 490.f) {
    return color * vec3(0.f, 1.f - (490.f - wavelength) / 50.f, 1.f);
  } else if (wavelength >= 490.f && wavelength < 510.f) {
    return color * vec3(0.f, 1.f, (510.f - wavelength) / 20.f);
  } else if (wavelength >= 510.f && wavelength < 580.f) {
    return color * vec3(1.f - ((580.f - wavelength) / 70.f), 1.f, 0.f);
  } else if (wavelength >= 580.f && wavelength < 640.f) {
    return color * vec3(1.f, (640.f - wavelength) / 60.f, 0.f);
  } else if (wavelength >= 640 && wavelength < 700) {
    return color * vec3(1.f, 0.f, 0.f);
  } else if (wavelength >= 700 && wavelength < 780.f) {
    return color * vec3(0.35f - 0.65f * ((780.f - wavelength) / 80.f), 0.f, 0.f);
  } else {
    return color;
  }
}

float skinBaseAbsorption(float lambda) {
  return 7.84 * pow(10.0, 8.0) * pow(lambda, -3.255);
}

float absorpCoeff(float mu_oxy, float mu_deoxy, float mu_fat, float mu_water, float lambda, float B, float S, float F, float W, float M) {
  float mu_melanosome = -362.0 * log(1+(lambda-380)*0.5) + 2000.0;
  mu_oxy = 2.303 * mu_oxy * (B / 64500.0);
  mu_deoxy = 2.303 * mu_deoxy * (B / 64500.0);
  mu_fat = mu_fat / 100.0;
  mu_water = mu_water / 100.0;

  return B * S * mu_oxy + B * (1.0 - S) * mu_deoxy + W * mu_water + F * mu_fat + M * mu_melanosome;
}

float scatCoeff(float lambda) {
  float alpha = 1.0;
  return alpha * (pow(lambda / 500, -3));
}

float decode(vec3 rgb) {
  int res = int(rgb.b*255.0);
  res = (res << 8) + int(rgb.g*255.0);
  res = (res << 8) + int(rgb.r*255.0);
  return float(res);
}

vec4 params(float lambda) {
  float idx = (lambda - 380.0)/2.0;
  float fat = decode(texture(uParamTex, vec2(idx, 0)).rgb);
  float water = decode(texture(uParamTex, vec2(idx, 1)).rgb);
  float hb = decode(texture(uParamTex, vec2(idx, 2)).rgb);
  float hbo2 = decode(texture(uParamTex, vec2(idx, 3)).rgb);

  return vec4(fat, water, hb, hbo2);
}

void main() {
  gPos = vFragPos;
  gUV = vUV;
  if (uHasNormalMap) {
    gNormal = texture(uNormalMap, vUV).rgb * 2.0 - 1.0;
    gNormal = normalize(vTBN * gNormal);
  } else {
    gNormal = vNormal;
  }

  vec3 albedo = texture(uAlbedoMap, vUV).rgb;
  if (uGammaCorrect)
    albedo = pow(albedo, vec3(2.2));

  if (uUseDynamicSkinColor) {
  //########################## old
    /*vec2 skinParams = texture(uSkinParamTex, vUV).rg;

    float melanin = 0.0135;
    float hemoglobin = 0.02;

    const float oneThird = 0.33333333;
    vec2 skinColorUV = vec2(pow(melanin, oneThird), pow(hemoglobin, oneThird));
    gAlbedo = albedo * texture(uSkinColorLookupTex, skinColorUV).rgb;*/
  //##########################
  float d = 0.0003; // skin depth

  for(int wl = 380; wl < 780; wl+=10) { // iteration over wavelengths
    vec4 rwl = params(wl); // get parameters for specific wl
    float A = (d * absorpCoeff(rwl.a, rwl.b, rwl.r, rwl.g, wl, uB, uS, uF, uW, uM))/2.303; // absorbance
    float S = scatCoeff(wl); // scattering (not used rn)
    float T = 1.0/pow(10.0, A); // transmittance
    float R = (1.0 - (A + T))*400 + 688; // reflectance
    gAlbedo += 1 - absorb(vec3(1), R );
  }
  gAlbedo /= 40.0;
  } else {
    gAlbedo = albedo;
  }

  gIrradiance = (uLight.color * uLight.intensity) * max(dot(gNormal, -uLight.direction), 0.0);
  if (uUseEnvIrradiance)
    gIrradiance += texture(uEnvIrradianceMap, gNormal).rgb;

  // Apply the albedo to the irradiance.
  gIrradiance *= gAlbedo;
}
