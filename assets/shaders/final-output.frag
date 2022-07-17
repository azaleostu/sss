#version 460

in vec2 vUV;

out vec4 fColor;

layout(binding = 0) uniform sampler2D uColorTex;

uniform bool uGammaCorrect;

void main() {
  vec3 color = texture(uColorTex, vUV).rgb;
  if (uGammaCorrect) {
    color = color / (color + 1.0);
    color = pow(color, vec3(1.0 / 2.2));
  }

  fColor = vec4(color, 1.0);
}
