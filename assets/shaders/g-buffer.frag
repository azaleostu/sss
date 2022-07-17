#version 460

in vec3 vNormal;
in vec3 vFragPos;
in vec2 vUV;
in mat3 vTBN;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec2 gUV;
layout(location = 2) out vec3 gNormal;
layout(location = 3) out vec3 gIrradiance;

layout(binding = 0) uniform samplerCube uEnvIrradianceMap;
layout(binding = 1) uniform sampler2D uNormalMap;

uniform bool uHasNormalMap;

struct Light {
  vec3 position;
  vec3 direction;
};

uniform Light uLight;

uniform bool uUseEnvIrradiance;

void main() {
  gPos = vFragPos;
  gUV = vUV;
  if (uHasNormalMap) {
    gNormal = texture2D(uNormalMap, vUV).rgb * 2.0 - 1.0;
    gNormal = normalize(vTBN * gNormal);
  } else {
    gNormal = vNormal;
  }

  gIrradiance = vec3(max(dot(gNormal, -uLight.direction), 0.0));
  if (uUseEnvIrradiance)
    gIrradiance += texture(uEnvIrradianceMap, gNormal).rgb;
}
