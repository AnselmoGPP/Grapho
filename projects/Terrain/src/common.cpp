
#include "common.hpp"

#include <glm/glm.hpp>


#if defined(__unix__)
const std::string SHADERS_DIR("../../../projects/Terrain/shaders/SPIRV/");
const std::string MODELS_DIR("../../../models/");
const std::string TEXTURES_DIR("../../../textures/");
#elif _WIN64 || _WIN32
const std::string SHADERS_DIR("../../../projects/Terrain/shaders/SPIRV/");
const std::string MODELS_DIR("../../../models/");
const std::string TEXTURES_DIR("../../../textures/");
#endif

float dayTime;
float sunAngDist = 3.14 / 10;
Light sunLight;

FreePolarCam camera_1(
	glm::vec3(0.f, 0.f, 20.0f),				// camera position
	50.f, 0.001f, 5.f,						// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,						// FOV, minFOV, maxFOV
	glm::vec3(90.f, 0.f, 0.f),				// Yaw (z), Pitch (x), Roll (y)
	0.1f, 5000.f,							// near & far view planes
	glm::vec3(0.0f, 0.0f, 1.0f) );			// world up

SphereCam camera_2(
	100.f, 0.002f, 5.f,						// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,						// FOV, minFOV, maxFOV
	0.1f, 5000.f,							// near & far view planes
	glm::vec3(0.f, 0.f, 1.f),				// world up
	glm::vec3(0.f, 0.f, 0.f),				// nucleus
	2000.f,									// radius
	45.f, 45.f );							// longitude & latitude

PlaneCam camera_3(
	glm::vec3(0.f, 0.f, 100.f),			// camera position
	50.f, 0.001f, 5.f,						// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,						// FOV, minFOV, maxFOV
	glm::vec3(0.f, -90.f, 0.f),				// Yaw (z), Pitch (x), Roll (y)
	0.1f, 5000.f );							// near & far view planes

//PlaneBasicCam camera_3;