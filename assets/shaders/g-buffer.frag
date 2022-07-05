#version 460

in vec3 vNormal;
in vec3 vFragPos;
in vec2 vUV;
in mat3 vTBN;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec2 gUV;
layout(location = 2) out vec3 gNormal;
layout(location = 3) out vec3 gAlbedo;
layout(location = 4) out vec3 gSpec;
layout(location = 5) out vec3 gIrradiance;

layout(binding = 1) uniform sampler2D uAlbedoTex;
layout(binding = 2) uniform sampler2D uSpecularTex;
layout(binding = 3) uniform sampler2D uNormalMap;

uniform bool uHasAlbedoTex;
uniform bool uHasSpecularTex;
uniform bool uHasNormalMap;

uniform vec3 uFallbackAlbedo;
uniform vec3 uFallbackSpecular;

void main() {
  gPos = vFragPos;
  gUV = vUV;
  if (uHasNormalMap) {
    gNormal = texture2D(uNormalMap, vUV).rgb * 2.0 - 1.0;
    gNormal = normalize(vTBN * gNormal);
  } else {
    gNormal = vNormal;
  }

  if (uHasAlbedoTex) {
    gAlbedo = texture(uAlbedoTex, vUV).rgb;
  } else {
    gAlbedo = uFallbackAlbedo;
  }

  if (uHasSpecularTex) {
    gSpec = texture(uSpecularTex, vUV).rrr;
  } else {
    gSpec = uFallbackSpecular;
  }

  gIrradiance = vec3(1.0);
}
