#version 450

layout( location = 0 ) in vec3 aVertexPosition;
layout( location = 1 ) in vec3 aVertexNormal;
layout( location = 2 ) in vec2 aVertexTexCoords;
layout( location = 3 ) in vec3 aVertexTangent;
layout( location = 4 ) in vec3 aVertexBitangent;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

uniform mat4 uMVMatrix;
uniform mat4 uNormalMatrix;

out vec3 aVNormal;
out vec3 aFragViewPos;
out vec2 aUV;
out mat3 aTBN;

out float aViewspaceDist;

void main()
{
    aUV = aVertexTexCoords;
    aVNormal = normalize(uNormalMatrix * vec4(aVertexNormal, 0.f )).xyz;
    aFragViewPos = (uMVMatrix * vec4(aVertexPosition, 1.f)).xyz;

    vec4 modelspace = vec4(aVertexPosition,1.f);

    vec4 worldspace = uModelMatrix * modelspace;

    vec4 viewspace = uViewMatrix * worldspace;

    vec4 screenspace = uProjectionMatrix * viewspace;

    aViewspaceDist = screenspace.z;

    vec3 T = normalize( ( uMVMatrix * vec4( aVertexTangent, 0.0 ) ).xyz );
    vec3 N = normalize( ( uMVMatrix * vec4( aVertexNormal, 0.0 ) ).xyz );
    vec3 B = normalize( ( uMVMatrix * vec4( aVertexBitangent, 0.0 ) ).xyz );

    aTBN = mat3( T, B, N );

    gl_Position = screenspace;
}
