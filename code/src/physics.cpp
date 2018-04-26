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

//Documentación
//http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/
//https://stackoverflow.com/questions/8844585/glm-rotate-usage-in-opengl
//https://stackoverflow.com/questions/11253930/how-can-i-find-out-the-vertex-coordinates-of-a-rotating-cube
//https://www.cs.cmu.edu/~baraff/sigcourse/notesd1.pdf
//https://www.scss.tcd.ie/Michael.Manzke/CS7057/cs7057-1516-09-CollisionResponse-mm.pdf

//Tareas
	//Fer Rand Decimals								-> OK
	//Establir vertex								-> OK
	//Detectar col.lisions dels vertexs amb parets	-> OK
	//Establir nou estat després de col.lisió
		//vertex que colisiona						-> OK
		//time de colision amb una tolerancia
		//contact point at collision point
		//response velocities at tc to prevent interpenetration
		//simulate from tc to t + dif t

	//arreglar t

bool show_test_window = false;
bool reset = false;
bool pause = false;

const float M = 1.f;							//Masa
const glm::vec3 gravity{ 0.f, -9.81f, 0.f };	//gravity value
const glm::vec3 initPos{ 0.f, 5.f, 0.f };		//initial cube position
const float halfW = 0.5f;						//half edge size

float tc;			//time collision
glm::vec3 lastX;
glm::quat lastQ;
glm::vec3 lastF;		
glm::vec3 lastL;		
glm::vec3 lastP;

glm::vec3 F;		//force
glm::vec3 x;		//position of cube center in world location
glm::vec3 p;		//p = R*pi + cubePos  //any position of the cube
glm::vec3 t;		//torque
glm::vec3 L;		//angular momentum
glm::vec3 P;		//linear momentum
glm::vec3 v;		//linear velocity, velocity of the cubePos
glm::vec3 w;		//angular velocity
glm::mat3 Ibody;	//inertias tensor
glm::mat3 invIbody;	// " " " " " " "
glm::mat3 invI;		// " " " " " " "

glm::quat q;		//quaternion
glm::mat4 tMat;		//translation matrix

const float tolerance = 0.25;

//initial Verts location
glm::vec3 initVerts[] = {
	glm::vec3(-halfW, -halfW, -halfW),		//   4---------7
	glm::vec3(-halfW, -halfW,  halfW),		//  /|        /|
	glm::vec3(halfW, -halfW,  halfW),		// / |       / |
	glm::vec3(halfW, -halfW, -halfW),		//5---------6  |
	glm::vec3(-halfW,  halfW, -halfW),		//|  0------|--3
	glm::vec3(-halfW,  halfW,  halfW),		//| /       | /
	glm::vec3(halfW,  halfW,  halfW),		//|/        |/
	glm::vec3(halfW,  halfW, -halfW)		//1---------2
};

glm::vec3 verts[8];	//updated verts 
glm::vec3 lastVerts[8]; //last verts

void resetVariables(); //forward declaration

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

	/////////////////////////////////////////Cube Position
	x = initPos;

	/////////////////////////////////////////Cube Vertex
	for (int i = 0; i < 8; i++)
	{
		verts[i] = x + initVerts[i];
	}

	/////////////////////////////////////////I matrix
	Ibody[0][0] = 1.f / 12.f * M *(pow(halfW * 2, 2) + pow(halfW * 2, 2));
	Ibody[1][1] = 1.f / 12.f * M *(pow(halfW * 2, 2) + pow(halfW * 2, 2));
	Ibody[2][2] = 1.f / 12.f * M *(pow(halfW * 2, 2) + pow(halfW * 2, 2));

	invIbody = glm::inverse(Ibody);

	/////////////////////////////////////////Force
	F = glm::vec3{
		-1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (2))),
		-1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (2))),
		-1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (2))),
	};

	/////////////////////////////////////////Torque
	p = x + q * glm::vec3(0, 1/2, 0);
	//t = glm::cross((p - cubePos), F);	//!!!!!!!!!!!!!!!!!!!!!!!!!!

	t += glm::vec3{
		-0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1))),
		-0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1))),
		-0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1)))
	};

	//////////////////////////////////////////Set up geometry
	Cube::setupCube();
	Sphere::setupSphere();
}

void PhysicsUpdate(float dt) {
	if (!pause) {
	
		//////////////////////////////////////////Euler - equations of motion
		lastF = F;
		F += gravity * dt;
		lastP = P;
		P += dt * F;
		lastL = L;
		L += dt * t;
		v = P / M;
		lastX = x;
		x += dt * v;
			invI = glm::mat3_cast(q) * invIbody * glm::transpose(glm::mat3_cast(q));
			w = invI * L;
		lastQ = q;
		q += dt * (0.5f * glm::quat(0, w) * q);		

		q = glm::normalize(q);

		//////////////////////////////////////////Transformations

		tMat = glm::translate(glm::mat4(1.f), x);

		//v' = R * v     ---\/ actualizar vertex segons rotació
		for (int i = 0; i < 8; i++)
			lastVerts[i] = verts[i];

		for (int i = 0; i < 8; i++) 
			verts[i] = glm::mat3_cast(q) * initVerts[i] + x;

		//////////////////////////////////////////Collision

		for (int i = 0; i < 8; i++)
		{
			if (verts[i].x < -5)
			{
				tc = dt;
				float edge = -5;

				if (((edge - tolerance) > verts[i].x && (edge + tolerance) < verts[i].x))	//esta dins
				{
					//calcular velocitats
				}
				else
				{
					while (((edge - tolerance) < verts[i].x && (edge + tolerance) > verts[i].x))	//esta fora
					{
						tc = tc / 2;

						lastF += gravity * dt;
						lastP += dt * lastF;
						lastL += dt * t;
						v = lastP / M;
						lastX += dt * v;
						invI = glm::mat3_cast(q) * invIbody * glm::transpose(glm::mat3_cast(q));
						w = invI * L;
						lastQ += dt * (0.5f * glm::quat(0, w) * q);

						lastQ = glm::normalize(lastQ);

						lastVerts[i] = glm::mat3_cast(lastQ) * initVerts[i] + lastX;

						//////ver que mitad está más cerca y set el time correspondiente * 2
					}
				}
				
			}
			else if (verts[i].x > 5) 
			{
				pause = true;
			}
			else if (verts[i].y < 0)
			{
				pause = true;
			}
			else if (verts[i].y > 10)
			{
				pause = true;
			}
			else if (verts[i].z < -5)
			{
				pause = true;
			}
			else if (verts[i].z > 5)
			{
				pause = true;
			}
		}
	}

	if (reset)	//reset things
	{
		pause = false;

		resetVariables();

		reset = false;
	}

	Cube::updateCube(tMat * glm::mat4_cast(q));
	Cube::drawCube();

	Sphere::updateSphere(verts[0], 0.1);		//vertex
	Sphere::drawSphere();
}

void PhysicsCleanup() {
	Cube::cleanupCube();
	Sphere::cleanupSphere();
}

void resetVariables() 
{
	x = initPos;

	v = { 0, 0, 0 };
	L = { 0, 0, 0 };
	P = { 0, 0, 0 };
	w = { 0, 0, 0 };

	q = { 0, 0, 0, 0 };
	tMat = glm::translate(glm::mat4(1.f), x);

	F = glm::vec3{
		-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10))),
		-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10))),
		-5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10)))
	};

	for (int i = 0; i < 8; i++)
		verts[i] = x + initVerts[i];

	t += glm::vec3{
		-0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1))),
		-0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1))),
		-0.5 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1)))
	};

}