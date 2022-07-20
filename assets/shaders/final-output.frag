#version 460

in vec2 vUV;

out vec4 fColor;

layout(binding = 0) uniform sampler2D uColorTex;

uniform bool uGammaCorrect;
uniform float uExposure;

void main() {
  vec3 color = texture(uColorTex, vUV).rgb;
  color = vec3(1.0) - exp(-color * uExposure);
  if (uGammaCorrect)
    color = pow(color, vec3(1.0 / 2.2));

  fColor = vec4(color, 1.0);
}
