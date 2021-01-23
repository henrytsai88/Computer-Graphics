#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
	// EDIT
	Lighting = 6,
	Shininess = 7,
};

GLint iLocMVP;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;

};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

Shape quad;
Shape m_shpae;
int cur_idx = 0; // represent which model should be rendered now

// EDIT
int mode = 0;
int light_idx = 0;
float shininess = 64;
GLuint iLocLightIdx;
GLuint iLocKa;
GLuint iLocKd;
GLuint iLocKs;
GLuint iLocView;
GLuint iLocShininess;

struct iLocLightInfo
{
	GLuint position;
	GLuint ambient;
	GLuint diffuse;
	GLuint specular;
	GLuint spotDirection;
	GLuint spotCutoff;
	GLuint spotExponent;
	GLuint constantAttenuation;
	GLuint linearAttenuation;
	GLuint quadraticAttenuation;
} iLocLightInfo[3];

struct LightInfo
{
	Vector3 position;
	Vector3 spotDirection;
	Vector3 ambient;
	Vector3 diffuse;
	Vector3 specular;
	float spotExponent;
	float spotCutoff;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
} lightInfo[3];

static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1
	);

	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	Vector3 P1P2 = main_camera.center - main_camera.position;
	Vector3 P1P3 = main_camera.up_vector - main_camera.position;
	GLfloat p1p2[3] = { P1P2.x, P1P2.y, P1P2.z };
	GLfloat p1p3[3] = { P1P3.x, P1P3.y, P1P3.z };
	GLfloat Rx[3], Ry[3], Rz[3];
	Rz[0] = -p1p2[0];
	Rz[1] = -p1p2[1];
	Rz[2] = -p1p2[2];
	Cross(p1p2, p1p3, Rx);
	Cross(Rz, Rx, Ry);
	Normalize(Rx);
	Normalize(Ry);
	Normalize(Rz);

	Matrix4 R = Matrix4(
		Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1
	);
	Matrix4 T = Matrix4(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1
	);
	view_matrix = R * T;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
	project_matrix = Matrix4(
		2 / (proj.right - proj.left), 0, 0, -((proj.right + proj.left) / (proj.right - proj.left)),
		0, 2 / (proj.top - proj.bottom), 0, -((proj.top + proj.bottom) / (proj.top - proj.bottom)),
		0, 0, -2 / (proj.farClip - proj.nearClip), -((proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip)),
		0, 0, 0, 1
	);
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;
	GLfloat f = -cos(proj.fovy / 2) / sin(proj.fovy / 2);
	project_matrix = Matrix4(
		f / proj.aspect, 0, 0, 0,
		0, f, 0, 0,
		0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), (2 * proj.farClip * proj.nearClip) / (proj.nearClip - proj.farClip),
		0, 0, -1, 0
	);
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	// [TODO] change your aspect ratio
	proj.aspect = (float)width / (float)height;
}

void drawPlane()
{
	GLfloat vertices[18]{ 1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0 };

	GLfloat colors[18]{ 0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,0.5,0.8,
		0.0,1.0,0.0 };


	// [TODO] draw the plane with above vertices and color
	/*glBindVertexArray(quad.vao);

	// 0 for vertices
	glBindBuffer(GL_ARRAY_BUFFER, quad.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// 1 for colors
	glBindBuffer(GL_ARRAY_BUFFER, quad.p_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	// re-define MVP with no geometric transformations
	Matrix4 MVP = project_matrix * view_matrix;
	GLfloat mvp[16];
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glBindVertexArray(quad.vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);*/
}

// EDIT
void updateLighting() {
	glUniform1i(iLocLightIdx, light_idx);
	glUniform1f(iLocShininess, shininess);

	// Directional light
	glUniform3f(iLocLightInfo[0].position, lightInfo[0].position.x, lightInfo[0].position.y, lightInfo[0].position.z);
	glUniform3f(iLocLightInfo[0].ambient, lightInfo[0].ambient.x, lightInfo[0].ambient.y, lightInfo[0].ambient.z);
	glUniform3f(iLocLightInfo[0].diffuse, lightInfo[0].diffuse.x, lightInfo[0].diffuse.y, lightInfo[0].diffuse.z);
	glUniform3f(iLocLightInfo[0].specular, lightInfo[0].specular.x, lightInfo[0].specular.y, lightInfo[0].specular.z);
	
	// Point light
	glUniform3f(iLocLightInfo[1].position, lightInfo[1].position.x, lightInfo[1].position.y, lightInfo[1].position.z);
	glUniform3f(iLocLightInfo[1].ambient, lightInfo[1].ambient.x, lightInfo[1].ambient.y, lightInfo[1].ambient.z);
	glUniform3f(iLocLightInfo[1].diffuse, lightInfo[1].diffuse.x, lightInfo[1].diffuse.y, lightInfo[1].diffuse.z);
	glUniform3f(iLocLightInfo[1].specular, lightInfo[1].specular.x, lightInfo[1].specular.y, lightInfo[1].specular.z);
	glUniform1f(iLocLightInfo[1].constantAttenuation, lightInfo[1].constantAttenuation);
	glUniform1f(iLocLightInfo[1].linearAttenuation, lightInfo[1].linearAttenuation);
	glUniform1f(iLocLightInfo[1].quadraticAttenuation, lightInfo[1].quadraticAttenuation);

	// Spot light
	glUniform3f(iLocLightInfo[2].position, lightInfo[2].position.x, lightInfo[2].position.y, lightInfo[2].position.z);
	glUniform3f(iLocLightInfo[2].diffuse, lightInfo[2].diffuse.x, lightInfo[2].diffuse.y, lightInfo[2].diffuse.z);
	glUniform3f(iLocLightInfo[2].ambient, lightInfo[2].ambient.x, lightInfo[2].ambient.y, lightInfo[2].ambient.z);
	glUniform3f(iLocLightInfo[2].specular, lightInfo[2].specular.x, lightInfo[2].specular.y, lightInfo[2].specular.z);
	glUniform3f(iLocLightInfo[2].spotDirection, lightInfo[2].spotDirection.x, lightInfo[2].spotDirection.y, lightInfo[2].spotDirection.z);
	glUniform1f(iLocLightInfo[2].spotExponent, lightInfo[2].spotExponent);
	glUniform1f(iLocLightInfo[2].spotCutoff, lightInfo[2].spotCutoff);
	glUniform1f(iLocLightInfo[2].constantAttenuation, lightInfo[2].constantAttenuation);
	glUniform1f(iLocLightInfo[2].linearAttenuation, lightInfo[2].linearAttenuation);
	glUniform1f(iLocLightInfo[2].quadraticAttenuation, lightInfo[2].quadraticAttenuation);
}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix
	MVP = project_matrix * view_matrix * T * R * S;
	// [TODO] row-major ---> column-major

	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];  mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];  mvp[15] = MVP[15];

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);

	// EDIT
	updateLighting();
	// Per-vertex lighting
	glUniform1i(mode, 0);
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++) 
	{
		PhongMaterial mat = models[cur_idx].shapes[i].material;
		glUniform3f(iLocKa, mat.Ka.x, mat.Ka.y, mat.Ka.z);
		glUniform3f(iLocKd, mat.Kd.x, mat.Kd.y, mat.Kd.z);
		glUniform3f(iLocKs, mat.Ks.x, mat.Ks.y, mat.Ks.z);
		glUniformMatrix4fv(iLocView, 1, GL_FALSE, view_matrix.getTranspose());

		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}
	// Per-pixel lighting
	glUniform1i(mode, 1);
	glViewport(WINDOW_WIDTH, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}

	// drawPlane();

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (action == 1) {
		switch (key) {
		case 'Z':
			cur_idx = (cur_idx == 0) ? 4 : cur_idx - 1;
			break;
		case 'X':
			cur_idx = (cur_idx == 4) ? 0 : cur_idx + 1;
			break;
		case 'T':
			cur_trans_mode = GeoTranslation;
			break;
		case 'R':
			cur_trans_mode = GeoRotation;
			break;
		case 'S':
			cur_trans_mode = GeoScaling;
			break;
		case 'I':
			cout << "spot_cutoff: " << lightInfo[2].spotCutoff << "\n";
			cout << "shininess: " << shininess << "\n";
			break;
		case 'L':
			light_idx = (light_idx == 2) ? 0 : light_idx + 1;
			cout << "current_light_sourse: ";
			if (light_idx == 0) cout << "directional light\n";
			else if (light_idx == 1) cout << "point light\n";
			else if (light_idx == 2) cout << "spot light\n";
			break;
		case 'K':
			cur_trans_mode = Lighting;
			break;
		case 'J':
			cur_trans_mode = Shininess;
			break;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	switch (cur_trans_mode) {
	case GeoTranslation:
		models[cur_idx].position.z += 0.1 * yoffset;
		break;
	case GeoRotation:
		models[cur_idx].rotation.z += 0.1 * yoffset;
		break;
	case GeoScaling:
		models[cur_idx].scale.z += 0.1 * yoffset;
		break;
	case Lighting:
		// Change diffuse intensity (directional/point)
		// Change cutoff angle (spot)
		if (light_idx == 0 || light_idx == 1) {
			lightInfo[light_idx].diffuse -= Vector3(0.1, 0.1, 0.1) * yoffset;
			if (lightInfo[light_idx].diffuse.x < 0) lightInfo[light_idx].diffuse = Vector3(0, 0, 0);
			if (lightInfo[light_idx].diffuse.x > 1) lightInfo[light_idx].diffuse = Vector3(1, 1, 1);
		}
		else if (light_idx == 2) {
			lightInfo[light_idx].spotCutoff -= 1.0 * yoffset;
			if (lightInfo[light_idx].spotCutoff < 0) lightInfo[light_idx].spotCutoff = 0;
			if (lightInfo[light_idx].spotCutoff > 90) lightInfo[light_idx].spotCutoff = 90;
		}
		break;
	case Shininess:
		// Change shininess
		shininess += yoffset;
		if (shininess < 0) shininess = 0;
		if (shininess > 450) shininess = 450;
		break;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	if (action == 0) mouse_pressed = false;
	else if (action == 1) mouse_pressed = true;
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	if (mouse_pressed) {
		double xchange = xpos - starting_press_x;
		double ychange = ypos - starting_press_y;

		switch (cur_trans_mode) {
		case GeoTranslation:
			models[cur_idx].position.x += 0.003 * xchange;
			models[cur_idx].position.y -= 0.003 * ychange;
			break;
		case GeoRotation:
			models[cur_idx].rotation.x += 0.01 * ychange;
			models[cur_idx].rotation.y += 0.01 * xchange;
			break;
		case GeoScaling:
			models[cur_idx].scale.x += 0.003 * xchange;
			models[cur_idx].scale.y += 0.003 * ychange;
			break;
		case Lighting:
			// Change light position
			lightInfo[light_idx].position.x += 0.003 * xchange;
			lightInfo[light_idx].position.y -= 0.003 * ychange;
			break;
		}
	}
	starting_press_x = (int)xpos;
	starting_press_y = (int)ypos;
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocMVP = glGetUniformLocation(p, "mvp");

	// EDIT
	mode = glGetUniformLocation(p, "mode");
	iLocView = glGetUniformLocation(p, "view");
	iLocLightIdx = glGetUniformLocation(p, "light_idx");
	iLocShininess = glGetUniformLocation(p, "shininess");

	// Materials
	iLocKa = glGetUniformLocation(p, "material.Ka");
	iLocKd = glGetUniformLocation(p, "material.Kd");
	iLocKs = glGetUniformLocation(p, "material.Ks");

	// Directional light
	iLocLightInfo[0].position = glGetUniformLocation(p, "light[0].position");
	iLocLightInfo[0].ambient = glGetUniformLocation(p, "light[0].La");
	iLocLightInfo[0].diffuse = glGetUniformLocation(p, "light[0].Ld");
	iLocLightInfo[0].specular = glGetUniformLocation(p, "light[0].Ls");
	iLocLightInfo[0].spotDirection = glGetUniformLocation(p, "light[0].spotDirection");
	iLocLightInfo[0].spotCutoff = glGetUniformLocation(p, "light[0].spotCutoff");
	iLocLightInfo[0].spotExponent = glGetUniformLocation(p, "light[0].spotExponent");
	iLocLightInfo[0].constantAttenuation = glGetUniformLocation(p, "light[0].constantAttenuation");
	iLocLightInfo[0].linearAttenuation = glGetUniformLocation(p, "light[0].linearAttenuation");
	iLocLightInfo[0].quadraticAttenuation = glGetUniformLocation(p, "light[0].quadraticAttenuation");

	// Point light
	iLocLightInfo[1].position = glGetUniformLocation(p, "light[1].position");
	iLocLightInfo[1].ambient = glGetUniformLocation(p, "light[1].La");
	iLocLightInfo[1].diffuse = glGetUniformLocation(p, "light[1].Ld");
	iLocLightInfo[1].specular = glGetUniformLocation(p, "light[1].Ls");
	iLocLightInfo[1].spotDirection = glGetUniformLocation(p, "light[1].spotDirection");
	iLocLightInfo[1].spotCutoff = glGetUniformLocation(p, "light[1].spotCutoff");
	iLocLightInfo[1].spotExponent = glGetUniformLocation(p, "light[1].spotExponent");
	iLocLightInfo[1].constantAttenuation = glGetUniformLocation(p, "light[1].constantAttenuation");
	iLocLightInfo[1].linearAttenuation = glGetUniformLocation(p, "light[1].linearAttenuation");
	iLocLightInfo[1].quadraticAttenuation = glGetUniformLocation(p, "light[1].quadraticAttenuation");

	// Spot light
	iLocLightInfo[2].position = glGetUniformLocation(p, "light[2].position");
	iLocLightInfo[2].ambient = glGetUniformLocation(p, "light[2].La");
	iLocLightInfo[2].diffuse = glGetUniformLocation(p, "light[2].Ld");
	iLocLightInfo[2].specular = glGetUniformLocation(p, "light[2].Ls");
	iLocLightInfo[2].spotDirection = glGetUniformLocation(p, "light[2].spotDirection");
	iLocLightInfo[2].spotCutoff = glGetUniformLocation(p, "light[2].spotCutoff");
	iLocLightInfo[2].spotExponent = glGetUniformLocation(p, "light[2].spotExponent");
	iLocLightInfo[2].constantAttenuation = glGetUniformLocation(p, "light[2].constantAttenuation");
	iLocLightInfo[2].linearAttenuation = glGetUniformLocation(p, "light[2].linearAttenuation");
	iLocLightInfo[2].quadraticAttenuation = glGetUniformLocation(p, "light[2].quadraticAttenuation");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	// EDIT

	// Direational light
	lightInfo[0].position = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[0].ambient = Vector3(0.15f, 0.15f, 0.15f);
	lightInfo[0].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[0].specular = Vector3(1.0f, 1.0f, 1.0f);

	// Point light
	lightInfo[1].position = Vector3(0.0f, 2.0f, 1.0f);
	lightInfo[1].ambient = Vector3(0.15f, 0.15f, 0.15f);
	lightInfo[1].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[1].specular = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[1].constantAttenuation = 0.01f;
	lightInfo[1].linearAttenuation = 0.8f;
	lightInfo[1].quadraticAttenuation = 0.1f;

	// Spot light
	lightInfo[2].position = Vector3(0.0f, 0.0f, 2.0f);
	lightInfo[2].ambient = Vector3(0.15f, 0.15f, 0.15f);
	lightInfo[2].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[2].specular = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[2].spotDirection = Vector3(0.0f, 0.0f, -1.0f);
	lightInfo[2].spotExponent = 50;
	lightInfo[2].spotCutoff = 30;
	lightInfo[2].constantAttenuation = 0.05f;
	lightInfo[2].linearAttenuation = 0.3f;
	lightInfo[2].quadraticAttenuation = 0.6f;

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
	for (int i = 0; i < 5; i++) LoadModels(model_list[i]);
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH * 2, WINDOW_HEIGHT, "106062314 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
