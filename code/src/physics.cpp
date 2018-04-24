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


//Mirar de ferho amb quaternions

//Fer Rand Decimals								-> OK
//Establir vertex						-----------------> Malament fet
//Detectar col.lisions dels vertexs amb parets	-> OK
//Establir nou estat després de col.lisió

bool show_test_window = false;
bool reset = false;
bool pause = false;

const float M = 1.f;

glm::vec3 gravity{ 0.f, -9.81f, 0.f };
glm::vec3 initPos{ 0.f, 5.f, 0.f };

float halfW = 0.5f; //tamany de la arista

glm::vec3 force;
glm::vec3 cubePos;
glm::vec3 torque;
glm::vec3 angMom;
glm::vec3 linMom;
glm::vec3 vel;
glm::mat3 inertiaTen, invInTen, inI;
glm::vec3 rot;
glm::mat3 rotMat;
glm::vec3 angVel;
glm::quat rotQuat;

glm::mat4 cubeMatPos = glm::translate(glm::mat4(1.f), cubePos);
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

	inertiaTen[0][0] = inertiaTen[1][1] = inertiaTen[2][2] = 1 / 12 * M*(pow(halfW * 2, 2) + pow(halfW * 2, 2));
	invInTen = glm::inverse(inertiaTen);

	force = glm::vec3{
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

													glm::quat rotQuat = glm::toQuat(cubeMatRotX * cubeMatRotY * cubeMatRotZ);


	Cube::setupCube();
	Sphere::setupSphere();
}

void PhysicsUpdate(float dt) {
	if (!pause) {
		
		/*force += gravity * dt;
		linMom += force * dt;
		torque += rotQuat - cubePos * force;
		angMom += angMom + torque * dt;
		vel = linMom / M;
		cubePos = cubePos + dt + vel;
		inI = rotMat * invInTen * glm::transpose(rotMat);
		angVel = inI * linMom;
		rotQuat = 0.5f * angVel * rotQuat;*/

		force += gravity * dt;
		linMom += force * dt;
		torque += (rot - cubePos) * force;
		angMom += torque * dt;
		vel = linMom / M;
		cubePos += dt * vel;
		inI = glm::toMat3(rotQuat) * invInTen * glm::transpose(glm::toMat3(rotQuat));
		angVel = inI * linMom;
		rotQuat = 0.5f * angVel * rotQuat;

		/*force += gravity * dt;
		linMom += force * dt;
		torque += (rot - cubePos) * force;
		angMom += angMom + torque * dt;
		vel = linMom / M;
		cubePos = cubePos + dt + vel;
		inI = rotMat * invInTen * glm::transpose(rotMat);
		angVel = inI * linMom;
		rotMat = rotMat + dt*(glm::cross(glm::toMat3(angVel), rotMat));*/

		//mirar de fer-ho amb una sola variable, tal com está feta la camera dels projectes d inf grafica

		cubeMatPos = glm::translate(glm::mat4(1.f), cubePos);

		/*cubeMatRotX = glm::rotate(cubeMatRotX, rot.x, glm::vec3{ 1, 0, 0 });
		cubeMatRotY = glm::rotate(cubeMatRotY, rot.y, glm::vec3{ 0, 1, 0 });
		cubeMatRotZ = glm::rotate(cubeMatRotZ, rot.z, glm::vec3{ 0, 0, 1 });

		glm::quat k = glm::toQuat(cubeMatRotX * cubeMatRotY * cubeMatRotZ);*/

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

		vel = { 0, 0, 0 };
		linMom = { 0, 0, 0 };

		force = glm::vec3{
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
	Cube::updateCube(cubeMatPos * glm::toMat4(rotQuat)); 

	Sphere::updateSphere(verts[0], 0.1);	//vertex

	Cube::drawCube();

	Sphere::drawSphere();
}

void PhysicsCleanup() {
	Cube::cleanupCube();
	Sphere::cleanupSphere();
}
