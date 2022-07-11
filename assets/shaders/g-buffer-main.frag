#version 460

in vec2 vUV;

out vec4 fColor;

layout(binding = 0) uniform sampler2D uLightShadowMap;

layout(binding = 1) uniform sampler2D uGBufPosTex;
layout(binding = 2) uniform sampler2D uGBufUVTex;
layout(binding = 3) uniform sampler2D uGBufNormalMap;

layout(binding = 4) uniform sampler2D uGBufIrradianceTex;
layout(binding = 5) uniform sampler2D uBlurredIrradianceTex;

layout(binding = 6) uniform sampler2D uAlbedoTex;

struct Light {
  vec3 position;
  vec3 direction;
  float farPlane;
};

uniform mat4 uLightVPMatrix;
uniform vec3 uCamPosition;
uniform Light uLight;

uniform bool uEnableTranslucency;
uniform bool uEnableBlur;
uniform float uTranslucency;
uniform float uSSSWeight;
uniform float uSSSWidth;
uniform float uSSSNormalBias;

// http://www.iryoku.com/translucency/
vec3 transmittance(vec3 pos, vec3 normal, vec3 lightDir) {
  float scale = 8.25 * (1.0 - uTranslucency) / uSSSWidth;

  vec4 shrunkPos = vec4(pos - 0.005 * normal, 1.0);
  vec4 shadowPos = uLightVPMatrix * shrunkPos;

  float d1 = texture(uLightShadowMap, (shadowPos.xy / shadowPos.w) * 0.5 + 0.5).r;
  float d2 = shadowPos.z * 0.5 + 0.5;
  float d = scale * abs(d1 - d2);

  float dd = -d * d;
  vec3 profile =
    vec3(0.233, 0.455, 0.649) * exp(dd / 0.0064) +
    vec3(0.1,   0.336, 0.344) * exp(dd / 0.0484) +
    vec3(0.118, 0.198, 0.0)   * exp(dd / 0.187)  +
    vec3(0.113, 0.007, 0.007) * exp(dd / 0.567)  +
    vec3(0.358, 0.004, 0.0)   * exp(dd / 1.99)   +
    vec3(0.078, 0.0,   0.0)   * exp(dd / 7.41);

  float approxBackCosTheta = clamp(uSSSNormalBias + dot(-normal, lightDir), 0.0, 1.0);
  return profile * approxBackCosTheta;
}

void main() {
  vec3 pos = texture(uGBufPosTex, vUV).rgb;
  vec2 UV = texture(uGBufUVTex, vUV).rg;
  vec3 normal = texture(uGBufNormalMap, vUV).rgb;

  vec3 irradiance = texture(uGBufIrradianceTex, vUV).rgb;
  vec3 blurredIrradiance = texture(uBlurredIrradianceTex, vUV).rgb;

  vec3 albedo = texture(uAlbedoTex, UV).rgb;

  const vec3 fragDir = normalize(uCamPosition - pos);
  const vec3 lightDir = -uLight.direction;

  vec3 transmittanceRes = vec3(0.0);
  if (uEnableTranslucency) {
    transmittanceRes = transmittance(pos, normal, lightDir) * albedo;
  }

  vec3 diffuse = irradiance * albedo;
  if (uEnableBlur) {
    diffuse = mix(diffuse, blurredIrradiance * albedo, uSSSWeight);
  }

  fColor = vec4(diffuse + transmittanceRes, 1.0);
}
