#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
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

//Fer Rand Decimals
//Establir vertex
//Detectar col.lisions dels vertexs amb parets
//Establir nou estat després de col.lisió

bool show_test_window = false;
bool reset = false;

glm::vec3 gravity{ 0.f, -9.81f, 0.f };
glm::vec3 initPos{ 0.f, 10.f, 0.f };

glm::vec3 force;
glm::vec3 cubePos;

glm::mat4 cubeMatPos = glm::translate(glm::mat4(1.f), cubePos);
glm::mat4 cubeMatRotX = glm::mat4(1.f);
glm::mat4 cubeMatRotY = glm::mat4(1.f);
glm::mat4 cubeMatRotZ = glm::mat4(1.f);

float rx, ry, rz; //random angles rotation

const float halfW = 0.5f; //tamany de la arista

namespace Cube {
	void setupCube();
	void cleanupCube();
	void updateCube(const glm::mat4& transform);
	void drawCube();

	glm::vec3 verts[];
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate

		if (ImGui::Button("Reset Simulation"))	reset = true;
	}

	ImGui::End();

	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

void PhysicsInit() {

	cubePos = initPos;
	force = glm::vec3{ static_cast <float> (rand() % 10 - 5), 
					   static_cast <float> (rand() % 10 - 5), 
					   static_cast <float> (rand() % 10 - 5) };

	rx = static_cast <float> (rand() % 1 - 0.5);
	ry = static_cast <float> (rand() % 1 - 0.5);
	rz = static_cast <float> (rand() % 1 - 0.5);

	Cube::setupCube();

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
}

void PhysicsUpdate(float dt) {

	force += gravity*dt;
	cubePos += force*dt;

	cubeMatPos = glm::translate(glm::mat4(1.f), cubePos);

	cubeMatRotX = glm::rotate(cubeMatRotX, rx, glm::vec3{ 1, 0, 0 });
	cubeMatRotY = glm::rotate(cubeMatRotY, ry, glm::vec3{ 0, 1, 0 });
	cubeMatRotZ = glm::rotate(cubeMatRotZ, rz, glm::vec3{ 0, 0, 1 });

	//https://stackoverflow.com/questions/11253930/how-can-i-find-out-the-vertex-coordinates-of-a-rotating-cube
	//v' = R * v     ---\/ actualizar vertex segons rotació

	glm::vec3 verts[] = {
		(cubePos + glm::vec3(-halfW, -halfW, -halfW)) * (cubeMatRotX * cubeMatRotY * cubeMatRotZ),
		(cubePos + glm::vec3(-halfW, -halfW,  halfW)),
		(cubePos + glm::vec3(halfW, -halfW,  halfW)),
		(cubePos + glm::vec3(halfW, -halfW, -halfW)),
		(cubePos + glm::vec3(-halfW,  halfW, -halfW)),
		(cubePos + glm::vec3(-halfW,  halfW,  halfW)),
		(cubePos + glm::vec3(halfW,  halfW,  halfW)),
		(cubePos + glm::vec3(halfW,  halfW, -halfW))
	};

	std::cout << verts[1].x << ", " << verts[1].y << ", " << verts[1].z << std::endl;

	if (reset)
	{
		//reset things

		cubePos = initPos;
		
		force = glm::vec3{ static_cast <float> (rand() % 10 - 5), static_cast <float> (rand() % 10 - 5), static_cast <float> (rand() % 10 - 5) };

		rx = static_cast <float> (rand() % 2 - 1);
		ry = static_cast <float> (rand() % 2 - 1);
		rz = static_cast <float> (rand() % 2 - 1);

		cubeMatPos = glm::translate(glm::mat4(1.f), cubePos);
		cubeMatRotX = glm::rotate(cubeMatRotX, rx, glm::vec3{ 1, 0, 0 });
		cubeMatRotY = glm::rotate(cubeMatRotY, ry, glm::vec3{ 0, 1, 0 });
		cubeMatRotZ = glm::rotate(cubeMatRotZ, rz, glm::vec3{ 0, 0, 1 });

		reset = false;
	}

	Cube::updateCube(cubeMatPos * (cubeMatRotX * cubeMatRotY * cubeMatRotZ));	//REVISAR ABANS D'ENTREGAR
	Cube::drawCube();
}

void PhysicsCleanup() {
	Cube::cleanupCube();
}