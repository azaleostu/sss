#version 460

in vec2 vUV;

out vec4 fColor;

layout(binding = 0) uniform sampler2D uLightShadowMap;

layout(binding = 1) uniform sampler2D uGBufPosTex;
layout(binding = 2) uniform sampler2D uGBufUVTex;
layout(binding = 3) uniform sampler2D uGBufNormalMap;
layout(binding = 4) uniform sampler2D uGBufAlbedoMap;
layout(binding = 5) uniform sampler2D uGBufIrradianceTex;

layout(binding = 6) uniform sampler2D uBlurredIrradianceTex;

struct Light {
  vec3 direction;
  vec3 color;
  float intensity;
  mat4 VPMatrix;
};

uniform Light uLight;

uniform bool uEnableTransmittance;
uniform bool uEnableBlur;

uniform float uTransmittanceStrength;
uniform float uSSSWeight;
uniform float uSSSWidth;
uniform float uSSSNormalBias;

// http://www.iryoku.com/translucency/
vec3 SSSTransmittance(vec3 pos, vec3 normal, vec3 lightDir, vec2 UVOffset) {
  float scale = 8.25 * (1.0 - uTransmittanceStrength) / uSSSWidth;

  vec4 shrunkPos = vec4(pos - 0.001 * normal, 1.0);
  vec4 shadowPos = uLight.VPMatrix * shrunkPos;

  float d1 = texture(uLightShadowMap, (shadowPos.xy / shadowPos.w) * 0.5 + 0.5 + UVOffset).r;
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
  vec3 normal = texture(uGBufNormalMap, vUV).rgb;
  vec3 albedo = texture(uGBufAlbedoMap, vUV).rgb;

  vec3 irradiance = texture(uGBufIrradianceTex, vUV).rgb;
  if (uEnableBlur)
    irradiance = mix(irradiance, texture(uBlurredIrradianceTex, vUV).rgb, uSSSWeight);

  vec3 transmittance = vec3(0.0);
  if (uEnableTransmittance) {
    const unsigned int NumSamples = 10;
    const float BlurWidth = 0.003;
    const float SampleDelta = BlurWidth / NumSamples;
    const float HalfWidth = BlurWidth / 2.0;

    for (float y = -HalfWidth; y < HalfWidth; y += SampleDelta) {
      for (float x = -HalfWidth; x < HalfWidth; x += SampleDelta)
        transmittance += SSSTransmittance(pos, normal, -uLight.direction, vec2(x, y)) * albedo;
    }

    transmittance /= (NumSamples * NumSamples);
  }

  fColor = vec4(irradiance + transmittance * (uLight.color * uLight.intensity), 1.0);
}
