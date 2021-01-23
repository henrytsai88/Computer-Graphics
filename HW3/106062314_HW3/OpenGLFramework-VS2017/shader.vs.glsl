#version 330

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec2 texCoord;
// EDIT
out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 _aPos;

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

uniform int light_idx;
uniform float shininess;

uniform mat4 um4p;	// projection matrix
uniform mat4 um4v;	// camera viewing transformation matrix
uniform mat4 um4m;	// rotation matrix

float dot(vec3 u, vec3 v) {
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

vec3 directionalLight(vec3 N, vec3 V){
	vec4 lightInView = um4v * vec4(light[0].position, 1.0f);
	vec3 S = normalize(lightInView.xyz + V);		 
	vec3 H = normalize(S + V);
	
	return light[0].La * material.Ka + dot(N, S) * light[0].Ld * material.Kd + pow(max(dot(N, H), 0), shininess) * light[0].Ls * material.Ks;
}

void main() 
{
	vec4 vertexInView = um4v * um4m * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	vec4 normalInView = transpose(inverse(um4v * um4m)) * vec4(aNormal, 0.0);

	vertex_normal = normalInView.xyz;

	vec3 N = normalize(vertex_normal);
	vec3 V = -vertexInView.xyz;

	vertex_color = directionalLight(N, V);
	_aPos = aPos;

	texCoord = aTexCoord;
	gl_Position = um4p * um4v * um4m * vec4(aPos, 1.0);
}
