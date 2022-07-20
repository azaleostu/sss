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
  if (uUseDynamicSkinColor) {
    vec2 skinParams = texture(uSkinParamTex, vUV).rg;

    float melanin = 0.0135;
    float hemoglobin = 0.02;

    const float oneThird = 0.33333333;
    vec2 skinColorUV = vec2(pow(melanin, oneThird), pow(hemoglobin, oneThird));
    gAlbedo = albedo * texture(uSkinColorLookupTex, skinColorUV).rgb;
  } else {
    gAlbedo = albedo;
  }

  // Not sure if this works with dynamic skin color.
  if (uGammaCorrect)
    gAlbedo = pow(gAlbedo, vec3(2.2));

  gIrradiance = (uLight.color * uLight.intensity) * max(dot(gNormal, -uLight.direction), 0.0);
  if (uUseEnvIrradiance)
    gIrradiance += texture(uEnvIrradianceMap, gNormal).rgb;
}
