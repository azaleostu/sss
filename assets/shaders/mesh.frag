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

layout(binding = 1) uniform sampler2D uDiffuseMap;
layout(binding = 2) uniform sampler2D uAmbientMap;
layout(binding = 3) uniform sampler2D uSpecularMap;
layout(binding = 4) uniform sampler2D uShininessMap;
layout(binding = 5) uniform sampler2D uNormalMap;

struct Light {
  vec3 position;
};

uniform vec3 uCamPosition;
uniform Light uLight;

void main() {
  const vec3 fragDir = normalize(uCamPosition - vFragPos);
  const vec3 lightDir = normalize(uLight.position - vFragPos);

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

  float shininessRes;
  if (uHasShininessMap) {
    shininessRes = texture(uShininessMap, vUV).x;
  } else {
    shininessRes = uShininess;
  }

  // Blinn-phong.
  const vec3 halfDir = normalize(fragDir + lightDir);
  const float spec = pow(max(dot(fragNormal, halfDir), 0.0), shininessRes);

  vec3 diffuseRes;
  vec3 ambientRes;
  vec3 specularRes;

  const float angle = max(dot(fragNormal, lightDir), 0.0);
  if (uHasDiffuseMap) {
    vec4 texel = texture(uDiffuseMap, vUV);
    if (texel.a < 0.5) {
      discard;
    } else {
      diffuseRes = angle * texel.rgb;
    }
  } else {
    diffuseRes = uDiffuse * angle;
  }

  if (uHasAmbientMap) {
    ambientRes = uAmbient * vec3(texture(uAmbientMap, vUV));
  } else {
    ambientRes = uAmbient;
  }

  if (uHasSpecularMap) {
    specularRes = spec * vec3(texture(uSpecularMap, vUV).rrr);
  } else {
    specularRes = uSpecular * spec;
  }

  vec3 lightRes = ambientRes + diffuseRes.xyz + specularRes;
  fragColor = vec4(lightRes, 1.f);
}
