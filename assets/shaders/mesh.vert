#version 450

layout(location = 0) in vec3 aVertexPosition;
layout(location = 1) in vec3 aVertexNormal;
layout(location = 2) in vec2 aVertexTexCoords;
layout(location = 3) in vec3 aVertexTangent;
layout(location = 4) in vec3 aVertexBitangent;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

uniform mat4 uMVMatrix;
uniform mat4 uNormalMatrix;

out vec3 vNormal;
out vec3 vFragViewPos;
out vec2 vUV;
out mat3 vTBN;

void main() {
  vFragViewPos = (uMVMatrix * vec4(aVertexPosition, 1.f)).xyz;
  vUV = aVertexTexCoords;
  vNormal = normalize(uNormalMatrix * vec4(aVertexNormal, 0.f)).xyz;

  vec3 T = normalize((uMVMatrix * vec4(aVertexTangent, 0.0)).xyz);
  vec3 N = normalize((uMVMatrix * vec4(aVertexNormal, 0.0)).xyz);
  vec3 B = normalize((uMVMatrix * vec4(aVertexBitangent, 0.0)).xyz);
  vTBN = mat3(T, B, N);

  gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aVertexPosition, 1.f);
}
