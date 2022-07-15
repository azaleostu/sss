#version 460

layout(location = 0) in vec3 aPos;

uniform mat4 uLightMVP;

void main() {
  vec4 pos = uLightMVP * vec4(aPos, 1.0);

  // Make sure the depth is linear.
  // https://github.com/iryoku/separable-sss/blob/master/Demo/Shaders/ShadowMap.fx
  pos.z *= pos.w;
  gl_Position = pos;
}
