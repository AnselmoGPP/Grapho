
#include "common.hpp"

#include <glm/glm.hpp>


float dayTime = 6.00;
float sunAngDist = 3.14 / 10;
Light sunLight;

FreePolarCam camera_1(
	glm::vec3(-1000.0f, -1000.0f, 1000.0f),	// camera position
	50.f, 0.001f, 5.f,						// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,						// FOV, minFOV, maxFOV
	glm::vec3(90.f, 0.f, 0.f),				// Yaw (z), Pitch (x), Roll (y)
	0.1f, 5000.f,							// near & far view planes
	glm::vec3(0.0f, 0.0f, 1.0f) );			// world up

SphereCam camera_2(
	100.f, 0.002f, 5.f,			// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,			// FOV, minFOV, maxFOV
	0.1f, 5000.f,				// near & far view planes
	glm::vec3(0.f, 0.f, 1.f),	// world up
	glm::vec3(0.f, 0.f, 0.f),	// nucleus
	2000.f,						// radius
	45.f, 45.f );				// longitude & latitude

//PlaneBasicCam camera_3;