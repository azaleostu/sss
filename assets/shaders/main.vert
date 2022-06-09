#version 460

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uModelMatrix;
uniform mat4 uMVPMatrix;
uniform mat4 uNormalMatrix;

out vec3 vNormal;
out vec3 vFragPos;
out vec2 vUV;
out mat3 vTBN;

void main() {
  vFragPos = (uModelMatrix * vec4(aPosition, 1.0)).xyz;

  vUV = aTexCoords;
  vNormal = normalize(uNormalMatrix * vec4(aNormal, 0.0)).xyz;

  vec3 T = normalize((uModelMatrix * vec4(aTangent, 0.0)).xyz);
  vec3 N = normalize((uModelMatrix * vec4(aNormal, 0.0)).xyz);
  vec3 B = normalize((uModelMatrix * vec4(aBitangent, 0.0)).xyz);
  vTBN = mat3(T, B, N);

  gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
}
