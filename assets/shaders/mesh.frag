#version 450

#define BLINN

layout( binding = 1 ) uniform sampler2D uDiffuseMap;
layout( binding = 2 ) uniform sampler2D uAmbiantMap;
layout( binding = 3 ) uniform sampler2D uSpecularMap;
layout( binding = 4 ) uniform sampler2D uShininessMap;
layout( binding = 5 ) uniform sampler2D uNormalMap;

uniform bool uHasAmbiantMap;
uniform bool uHasDiffuseMap;
uniform bool uHasSpecularMap;
uniform bool uHasShininessMap;
uniform bool uHasNormalMap;

uniform vec3 uAmbiant;
uniform vec3 uDiffuse;
uniform vec3 uSpecular;
uniform float uShininess;

layout( location = 0 ) out vec4 fragColor;

uniform vec3 uViewLightPos;

in vec3 aVNormal;
in vec3 aFragViewPos;
in vec2 aUV;
in float aViewspaceDist;
in mat3 aTBN;

void main()
{
	const vec3 viewDir = normalize(-aFragViewPos);
	const vec3 lightDir = normalize( uViewLightPos - aFragViewPos );

	vec3 fragNormal = vec3( 0.0 );
	if ( uHasNormalMap )
	{
		fragNormal = texture2D( uNormalMap, aUV ).rgb;
		fragNormal = fragNormal * 2.0 - 1.0;
		fragNormal = normalize( aTBN * fragNormal );
	}
	else
	{
		fragNormal = normalize( aVNormal );
		if ( dot( fragNormal, viewDir ) < 0.0 )
			fragNormal *= -1;
	}

	float shininessRes;
	if(uHasShininessMap) shininessRes = texture(uShininessMap, aUV).x;
	else shininessRes = uShininess;
	
#ifdef BLINN
	const vec3 halfDir = normalize( viewDir + lightDir );
	const float spec = pow( max( dot( fragNormal, halfDir ), 0.0 ), shininessRes );
#else
	const vec3 reflected = reflect( -lightDir, fragNormal );
	const float spec = pow( max( dot( reflected, viewDir ), 0.0 ), shininessRes );
#endif

	vec3 diffuseRES;
	vec3 ambiantRES;
	vec3 specularRES;

	const float angle = max( dot( fragNormal, lightDir ), 0.0 );
	if(uHasDiffuseMap) {
		vec4 texel = texture(uDiffuseMap, aUV);
		diffuseRES = angle * texel.rgb;
		if ( texel.a < 0.5 )
			discard;
	}
	else diffuseRES = uDiffuse * angle;

	if(uHasAmbiantMap) ambiantRES = uAmbiant * vec3(texture(uAmbiantMap, aUV));
	else ambiantRES = uAmbiant;

	if(uHasSpecularMap) specularRES = spec * vec3(texture(uSpecularMap, aUV).rrr);
	else specularRES = uSpecular * spec;

	vec3 lightRes = ambiantRES + diffuseRES.xyz + specularRES;

	fragColor = vec4(lightRes, 1.f);
}
