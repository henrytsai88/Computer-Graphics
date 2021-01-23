#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 _aPos;

uniform mat4 mvp;
uniform mat4 view;
uniform int light_idx;
uniform float shininess;

struct LightInfo{
	vec3 position;
	vec3 spotDirection;
	vec3 La;
	vec3 Ld;
	vec3 Ls;
	float spotExponent;
	float spotCutoff;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};
uniform LightInfo light[3];

struct MaterialInfo
{
	vec3 Ka;
	vec3 Kd;
	vec3 Ks;
};
uniform MaterialInfo material;

float dot(vec3 u, vec3 v) {
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

vec3 directionalLight(vec3 N, vec3 V){
	vec4 lightInView = view * vec4(light[0].position, 1.0f);
	vec3 S = normalize(lightInView.xyz + V);		 
	vec3 H = normalize(S + V);
	
	return light[0].La * material.Ka + dot(N, S) * light[0].Ld * material.Kd + pow(max(dot(N, H), 0), shininess) * light[0].Ls * material.Ks;
}

vec3 pointLight(vec3 N, vec3 V){
	vec4 lightInView = view * vec4(light[1].position, 1.0f);
	vec3 S = normalize(lightInView.xyz + V);		 
	vec3 H = normalize(S + V);

	vec4 vertexInView = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);

	float dist = length(vertexInView.xyz - lightInView.xyz);
	float f = min(1.0 / (light[1].constantAttenuation + light[1].linearAttenuation * dist + light[1].quadraticAttenuation * dist * dist), 1.0);
	
	return light[1].La * material.Ka + f * (dot(N, S) * light[1].Ld * material.Kd + pow(max(dot(N, H), 0), shininess) * light[1].Ls * material.Ks);
}

vec3 spotLight(vec3 N, vec3 V){
	vec4 lightInView = view * vec4(light[2].position, 1.0f);
	vec3 S = normalize(lightInView.xyz + V);		 
	vec3 H = normalize(S + V);

	vec4 vertexInView = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);

	float dist = length(vertexInView.xyz - lightInView.xyz);
	float f = min(1.0 / (light[2].constantAttenuation + light[2].linearAttenuation * dist + light[2].quadraticAttenuation * dist * dist), 1.0);
	
	float dotVD = dot(normalize(vertexInView.xyz - light[2].position), normalize(light[2].spotDirection));
    float spotLightEffect = (dotVD > cos(light[2].spotCutoff * 0.0174532925)) ? pow(max(dotVD, 0), light[2].spotExponent) : 0.0;

	return light[2].La * material.Ka + spotLightEffect * f * (dot(N, S) * light[2].Ld * material.Kd + pow(max(dot(N, H), 0), shininess) * light[2].Ls * material.Ks);
}

void main()
{
	vec4 vertexInView = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	vec4 normalInView = mvp * vec4(aNormal, 0.0);

	vertex_normal = normalInView.xyz;

	vec3 N = normalize(vertex_normal);
	vec3 V = -vertexInView.xyz;

	switch (light_idx) {
		case 0:
			vertex_color = directionalLight(N, V);
			break;
		case 1:
			vertex_color = pointLight(N, V);
			break;
		case 2:
			vertex_color = spotLight(N, V);
			break;
	}
	
	_aPos = aPos;
	gl_Position = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);
}

