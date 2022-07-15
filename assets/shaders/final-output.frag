#version 460

in vec2 vUV;
out vec4 fColor;

layout(binding = 0) uniform sampler2D uColorTex;

void main() {
  fColor = texture(uColorTex, vUV);
}
