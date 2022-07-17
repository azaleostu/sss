#version 460

layout(location = 0) in vec3 aPos;

out vec3 vPos;

uniform mat4 uProj;
uniform mat4 uView;

void main() {
  vPos = aPos;
  gl_Position = uProj * uView * vec4(aPos, 1.0);
}
