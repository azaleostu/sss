#version 460

layout(location = 0) in vec3 aPos;

uniform mat4 uProj;
uniform mat4 uView;

out vec3 vPos;

void main() {
  vPos = aPos;

  mat4 dirView = mat4(mat3(uView));
  gl_Position = uProj * dirView * vec4(aPos, 1.0);
}
