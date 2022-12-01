
#include "common.hpp"

#include <glm/glm.hpp>


#if defined(__unix__)
const std::string shadersDir("../../../projects/Terrain/shaders/GLSL/");
const std::string vertexDir("../../../models/");
const std::string texDir("../../../textures/");
#elif _WIN64 || _WIN32
const std::string shadersDir("../../../projects/Terrain/shaders/GLSL/");
const std::string vertexDir("../../../models/");
const std::string texDir("../../../textures/");
#endif


// Cameras --------------------------------------------------

FreePolarCam camera_1(
	glm::vec3(0.f, 0.f, 20.0f),		// camera position
	50.f, 0.001f, 5.f,				// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,				// FOV, minFOV, maxFOV
	glm::vec3(90.f, 0.f, 0.f),		// Yaw (z), Pitch (x), Roll (y)
	0.1f, 5000.f,					// near & far view planes
	glm::vec3(0.0f, 0.0f, 1.0f) );	// world up

SphereCam camera_2(
	100.f, 0.002f, 5.f,				// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,				// FOV, minFOV, maxFOV
	0.1f, 5000.f,					// near & far view planes
	glm::vec3(0.f, 0.f, 1.f),		// world up
	glm::vec3(0.f, 0.f, 0.f),		// nucleus
	4000.f,							// radius
	45.f, 45.f );					// latitude & longitude

PlaneCam2 camera_3(
	glm::vec3(0.f, 0.f, 1000.f),	// camera position
	50.f, 0.001f, 5.f,				// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,				// FOV, minFOV, maxFOV
	glm::vec3(0.f, -90.f, 0.f),		// Yaw (z), Pitch (x), Roll (y)
	0.1f, 5000.f );					// near & far view planes

PlanetFPcam camera_4(
	10.f, 0.001f, 5.f,				// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,				// FOV, minFOV, maxFOV
	0.1f, 5000.f,					// near & far view planes
	{ 0.f, 0.f, 0.f },				// nucleus
	1000,							// radius
	45.f, 0.f );					// latitude & longitude
