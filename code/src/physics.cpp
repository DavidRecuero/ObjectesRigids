#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cstdio>
#include <cassert>
#include <iostream>		//std::cout, endl...
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include "GL_framework.h"

//http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/
//https://stackoverflow.com/questions/8844585/glm-rotate-usage-in-opengl
//https://stackoverflow.com/questions/11253930/how-can-i-find-out-the-vertex-coordinates-of-a-rotating-cube

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//https://www.cs.cmu.edu/~baraff/sigcourse/notesd1.pdf


//Mirar de ferho amb quaternions

//Fer Rand Decimals								-> OK
//Establir vertex						-----------------> Malament fet
//Detectar col.lisions dels vertexs amb parets	-> OK
//Establir nou estat després de col.lisió

bool show_test_window = false;
bool reset = false;
bool pause = false;

const float M = 1.f;	//Masa

glm::vec3 gravity{ 0.f, -9.81f, 0.f };
glm::vec3 initPos{ 0.f, 5.f, 0.f };

float halfW = 0.5f; //tamany de la arista / 2

glm::vec3 F;		//force
glm::vec3 cubePos;	//position of cube center in world location
glm::vec3 p;		//p = R*pi + cubePos  //any position of the cube
glm::vec3 t;		//torque
glm::vec3 L;		//angular momentum
glm::vec3 P;		//linear momentum
glm::vec3 v;		//linear velocity, velocity of the cubePos
glm::mat3 Ibody;	//inertia tensor
glm::mat3 invIbody;
glm::mat3 invI;
glm::vec3 rot;		//rotation angles
glm::mat3 R;		//rotation matrix R
glm::vec3 w;		//angular velocity
glm::mat3 W;
glm::quat q;		//quaternion

glm::mat3 cubeMatPos = glm::translate(glm::mat4(1.f), cubePos);
glm::mat4 cubeMatRotX = glm::mat4(1.f);
glm::mat4 cubeMatRotY = glm::mat4(1.f);
glm::mat4 cubeMatRotZ = glm::mat4(1.f);

//   4---------7
//  /|        /|
// / |       / |
//5---------6  |
//|  0------|--3
//| /       | /
//|/        |/
//1---------2

glm::vec3 verts[] = {
	cubePos + glm::vec3(-halfW, -halfW, -halfW),
	cubePos + glm::vec3(-halfW, -halfW,  halfW),
	cubePos + glm::vec3(halfW, -halfW,  halfW),
	cubePos + glm::vec3(halfW, -halfW, -halfW),
	cubePos + glm::vec3(-halfW,  halfW, -halfW),
	cubePos + glm::vec3(-halfW,  halfW,  halfW),
	cubePos + glm::vec3(halfW,  halfW,  halfW),
	cubePos + glm::vec3(halfW,  halfW, -halfW)
};

glm::vec3 a[] = {
	glm::vec3(-halfW, -halfW, -halfW),
	glm::vec3(-halfW, -halfW,  halfW),
	glm::vec3(halfW, -halfW,  halfW),
	glm::vec3(halfW, -halfW, -halfW),
	glm::vec3(-halfW,  halfW, -halfW),
	glm::vec3(-halfW,  halfW,  halfW),
	glm::vec3(halfW,  halfW,  halfW),
	glm::vec3(halfW,  halfW, -halfW)
};

namespace Cube {
	void setupCube();
	void cleanupCube();
	void updateCube(const glm::mat4& transform);
	void drawCube();
}

namespace Sphere {
	extern void setupSphere(glm::vec3 pos = glm::vec3(0.f, 1.f, 0.f), float radius = 1.f);
	extern void cleanupSphere();
	extern void updateSphere(glm::vec3 pos, float radius = 1.f);
	extern void drawSphere();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate

		if (ImGui::Button("Reset Simulation"))	reset = true;

		ImGui::Checkbox("Pause", &pause);
	}

	ImGui::End();

	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

void PhysicsInit() {

	cubePos = initPos;

	Ibody[0][0] = Ibody[1][1] = Ibody[2][2] = 1 / 12 * M*(pow(halfW * 2, 2) + pow(halfW * 2, 2));
	invIbody = glm::inverse(Ibody);

	F = glm::vec3{
		-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10))),
		-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10))),
		-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10)))
	};

	rot.x = -0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1)));
	rot.y = -0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1)));
	rot.z = -0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1)));

	cubeMatRotX = glm::rotate(cubeMatRotX, rot.x, glm::vec3{ 1, 0, 0 });
	cubeMatRotY = glm::rotate(cubeMatRotY, rot.y, glm::vec3{ 0, 1, 0 });
	cubeMatRotZ = glm::rotate(cubeMatRotZ, rot.z, glm::vec3{ 0, 0, 1 });

	//R = cubeMatRotX * cubeMatRotY * cubeMatRotZ;
	q = glm::toQuat(R);

	//torque += glm::cross((glm::vec3{0, 0, 0} - cubePos), force);
	p = cubePos + R * glm::vec3(0, -halfW, 0);
	t = glm::cross((p - cubePos), F);

	Cube::setupCube();
	Sphere::setupSphere();
}

void PhysicsUpdate(float dt) {
	if (!pause) {

		//p = cubePos + R * glm::vec3(-halfW / 2, -halfW, halfW / 2);
		//t = glm::cross((p - cubePos), F);

		/*F += gravity * dt;
		P += F * dt;
		L += t * dt;
		v = P / M;
		cubePos += dt * v;
		invI = R * invIbody * glm::transpose(R);
		w = invI * L;

		W[0][0] = 0;	W[0][1] = -w.z;		W[0][2] = w.y;
		W[1][0] = w.z;	W[1][1] = 0;		W[1][2] = -w.x;
		W[2][0] = -w.y;	W[2][1] = w.x;		W[2][2] = 0;

		R = R + dt*(W * R); //R = R + dt * glm::cross(W, R);*/
		
		F += gravity * dt;
		P += F * dt;
		L += t * dt;
		v = P / M;
		cubePos += dt * v;
		invI = glm::toMat3(glm::normalize(q)) * invIbody * glm::transpose(glm::toMat3(glm::normalize(q)));
		w = invI * L;

		q += 0.5f*(w*q);		///algo falla aqui, creo!!!!!!

		cubeMatPos = glm::translate(glm::mat4(1.f), cubePos);

		//https://stackoverflow.com/questions/11253930/how-can-i-find-out-the-vertex-coordinates-of-a-rotating-cube
		//v' = R * v     ---\/ actualizar vertex segons rotació

		for (int i = 0; i < 8; i++) {
			a[i] = (cubeMatRotX * cubeMatRotY * cubeMatRotZ) * glm::vec4(a[i], 0);
			verts[i] = a[i] + cubePos;
		}

////////COLLISION

		for (int i = 0; i < 8; i++)
		{
			if (verts[i].x < -5)		pause = true;
			else if (verts[i].x > 5)	pause = true;
			else if (verts[i].y < 0)	pause = true;
			else if (verts[i].y > 10)	pause = true;
			else if (verts[i].z < -5)	pause = true;
			else if (verts[i].z > 5)	pause = true;
		}
	}

	if (reset)	//reset things
	{
		pause = false;

		cubePos = initPos;

		v = { 0, 0, 0 };
		L = { 0, 0, 0 };

		F = glm::vec3{
			-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10))),
			-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10))),
			-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10)))
		};

		rot.x = -0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1)));
		rot.y = -0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1)));
		rot.z = -0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1)));

		cubeMatPos = glm::translate(glm::mat4(1.f), cubePos);
		cubeMatRotX = glm::rotate(cubeMatRotX, rot.x, glm::vec3{ 1, 0, 0 });
		cubeMatRotY = glm::rotate(cubeMatRotY, rot.y, glm::vec3{ 0, 1, 0 });
		cubeMatRotZ = glm::rotate(cubeMatRotZ, rot.z, glm::vec3{ 0, 0, 1 });

		verts[0] = cubePos + glm::vec3(-halfW, -halfW, -halfW);
		verts[1] = cubePos + glm::vec3(-halfW, -halfW, halfW);
		verts[2] = cubePos + glm::vec3(halfW, -halfW, halfW);
		verts[3] = cubePos + glm::vec3(halfW, -halfW, -halfW);
		verts[4] = cubePos + glm::vec3(-halfW, halfW, -halfW);
		verts[5] = cubePos + glm::vec3(-halfW, halfW, halfW);
		verts[6] = cubePos + glm::vec3(halfW, halfW, halfW);
		verts[7] = cubePos + glm::vec3(halfW, halfW, -halfW);

		a[0] = glm::vec3(-halfW, -halfW, -halfW);
		a[1] = glm::vec3(-halfW, -halfW, halfW);
		a[2] = glm::vec3(halfW, -halfW, halfW);
		a[3] = glm::vec3(halfW, -halfW, -halfW);
		a[4] = glm::vec3(-halfW, halfW, -halfW);
		a[5] = glm::vec3(-halfW, halfW, halfW);
		a[6] = glm::vec3(halfW, halfW, halfW);
		a[7] = glm::vec3(halfW, halfW, -halfW);

		reset = false;
	}

	//Cube::updateCube(cubeMatPos * (cubeMatRotX * cubeMatRotY * cubeMatRotZ));	//REVISAR ABANS D'ENTREGAR
	//Cube::updateCube(cubeMatPos * glm::toMat4(rotQuat));
	//Cube::updateCube(cubeMatPos * R);	//REVISAR ABANS D'ENTREGAR
	Cube::updateCube(cubeMatPos * glm::toMat3(glm::normalize(q)));
	Sphere::updateSphere(verts[0], 0.1);			//vertex

	Cube::drawCube();

	Sphere::drawSphere();
}

void PhysicsCleanup() {
	Cube::cleanupCube();
	Sphere::cleanupSphere();
}
