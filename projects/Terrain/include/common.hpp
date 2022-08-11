#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

#include "ubo.hpp"
#include "camera.hpp"

// File's paths
#if defined(__unix__)
const std::string shaders_dir("../../../projects/Terrain/shaders/SPIRV/");
const std::string textures_dir("../../../textures/");
#elif _WIN64 || _WIN32
const std::string SHADERS_DIR("../../../projects/Terrain/shaders/SPIRV/");
const std::string MODELS_DIR("../../../models/");
const std::string TEXTURES_DIR("../../../textures/");
#endif

// Camera
extern FreePolarCam camera_1;
extern SphereCam camera_2;
extern PlaneCam camera_3;

// Sun & light
extern float dayTime;
extern float sunAngDist;
extern Light sunLight;

#endif


