#version 330

in vec2 texCoord;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 _aPos;

out vec4 fragColor;

// [TODO] passing texture from main.cpp
// Hint: sampler2D
uniform sampler2D tex;

// EDIT
uniform int mode;

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

uniform mat4 um4p;	// projection matrix
uniform mat4 um4v;	// camera viewing transformation matrix
uniform mat4 um4m;	// rotation matrix

uniform int light_idx;
uniform float shininess;

float dot(vec3 u, vec3 v) {
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

vec3 directionalLight(vec3 N, vec3 V){
	vec4 lightInView = um4v * vec4(light[0].position, 1.0f);
	vec3 S = normalize(lightInView.xyz + V);		 
	vec3 H = normalize(S + V);
	
	return light[0].La * material.Ka + dot(N, S) * light[0].Ld * material.Kd + pow(max(dot(N, H), 0), shininess) * light[0].Ls * material.Ks;
}

void main() {
	fragColor = vec4(vertex_color, 1);

	vec4 vertexInView = um4v * um4m * vec4(_aPos.x, _aPos.y, _aPos.z, 1.0);

	vec3 N = normalize(vertex_normal);
	vec3 V = -vertexInView.xyz;

	vec3 color = directionalLight(N, V);
	fragColor = (mode == 1) ? vec4(vertex_color, 1.0f) : vec4(color, 1.0f);

	// [TODO] sampleing from texture
	// Hint: texture
	fragColor = texture(tex, texCoord) * fragColor;
}
