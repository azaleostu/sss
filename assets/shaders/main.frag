#version 460

#define NUM_LIGHTS 1

in vec3 vNormal;
in vec3 vFragPos;
in vec2 vUV;
in mat3 vTBN;

layout(location = 0) out vec4 fragColor;

uniform bool uHasAmbientMap;
uniform bool uHasDiffuseMap;
uniform bool uHasSpecularMap;
uniform bool uHasShininessMap;
uniform bool uHasNormalMap;

uniform vec3 uAmbient;
uniform vec3 uDiffuse;
uniform vec3 uSpecular;
uniform float uShininess;

layout(binding = 0) uniform sampler2D uLightShadowMap;

layout(binding = 1) uniform sampler2D uDiffuseMap;
layout(binding = 2) uniform sampler2D uAmbientMap;
layout(binding = 3) uniform sampler2D uSpecularMap;
layout(binding = 4) uniform sampler2D uShininessMap;
layout(binding = 5) uniform sampler2D uNormalMap;

struct Light {
  vec3 position;
  vec3 direction;
  float farPlane;
};

uniform vec3 uCamPosition;
uniform mat4 uLightVPMatrix;
uniform Light uLight;

uniform bool uEnableTranslucency;
uniform float uTranslucency;
uniform float uSSSWidth;
uniform float uSSSNormalBias;

// See: http://www.iryoku.com/separable-sss/
vec3 transmittance(vec3 fragNormal, vec3 lightDir) {
  float scale = 8.25 * (1.0 - uTranslucency) / uSSSWidth;

  vec4 shrunkPos = vec4(vFragPos - 0.001 * fragNormal, 1.0);
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

  float approxBackCosTheta = clamp(uSSSNormalBias + dot(-fragNormal, lightDir), 0.0, 1.0);
  return profile * approxBackCosTheta;
}

void main() {
  const vec3 fragDir = normalize(uCamPosition - vFragPos);
  const vec3 lightDir = -uLight.direction;

  vec3 fragNormal = vec3(0.0);
  if (uHasNormalMap) {
    fragNormal = texture2D(uNormalMap, vUV).rgb;
    fragNormal = fragNormal * 2.0 - 1.0;
    fragNormal = normalize(vTBN * fragNormal);
  } else {
    fragNormal = normalize(vNormal);
    if (dot(fragNormal, fragDir) < 0.0) {
      fragNormal *= -1;
    }
  }

  float shininess;
  if (uHasShininessMap) {
    shininess = texture(uShininessMap, vUV).x;
  } else {
    shininess = uShininess;
  }

  // Blinn-phong.
  vec3 halfDir = normalize(fragDir + lightDir);
  float spec = pow(max(dot(fragNormal, halfDir), 0.0), shininess);

  vec3 diffuseColor;
  vec3 ambientColor;
  vec3 specularColor;

  float cosTheta = max(dot(fragNormal, lightDir), 0.0);
  if (uHasDiffuseMap) {
    vec4 texel = texture(uDiffuseMap, vUV);
    if (texel.a < 0.5) {
      discard;
    } else {
      diffuseColor = texel.rgb;
    }
  } else {
    diffuseColor = uDiffuse;
  }

  if (uHasAmbientMap) {
    ambientColor = texture(uAmbientMap, vUV).xyz;
  } else {
    ambientColor = uAmbient;
  }

  if (uHasSpecularMap) {
    specularColor = texture(uSpecularMap, vUV).rrr;
  } else {
    specularColor = uSpecular * spec;
  }

  vec3 ambientRes = ambientColor * diffuseColor;
  vec3 diffuseRes = cosTheta * diffuseColor;
  vec3 transmittanceRes = vec3(0.0);
  if (uEnableTranslucency) {
    transmittanceRes = transmittance(fragNormal, lightDir) * diffuseColor;
  }
  vec3 specularRes = specularColor * spec;

  vec3 lightRes = ambientRes + diffuseRes + transmittanceRes + specularRes;
  fragColor = vec4(lightRes, 1.0);
}
