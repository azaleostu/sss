#version 460

layout(location = 0) in vec3 aPos;

uniform mat4 uLightMVP;

void main() {
  vec4 pos = uLightMVP * vec4(aPos, 1.0);
  pos.z *= pos.w;
  gl_Position = pos;
}
