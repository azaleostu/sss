#version 460

in vec3 vPos;

out vec4 fColor;

layout(binding = 0) uniform samplerCube uCubeMap;

void main() {
  vec3 envColor = texture(uCubeMap, vPos).rgb;
  fColor = vec4(envColor, 1.0);
}
