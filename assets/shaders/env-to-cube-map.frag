#version 460

in vec3 vPos;

out vec4 fColor;

layout(binding = 0) uniform sampler2D uEnvColor;

// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
vec2 sphericalToUV(vec3 dir) {
  const vec2 invAtan = vec2(0.1591, 0.3183);
  return vec2(atan(dir.z, dir.x), asin(dir.y)) * invAtan + 0.5;
}

void main() {
  fColor = vec4(texture(uEnvColor, sphericalToUV(normalize(vPos))).rgb, 1.0);
}
