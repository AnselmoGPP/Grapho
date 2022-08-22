#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

#include "ubo.hpp"
#include "camera.hpp"

// File's paths
extern const std::string SHADERS_DIR;
extern const std::string MODELS_DIR;
extern const std::string TEXTURES_DIR;

// Camera
extern FreePolarCam camera_1;
extern SphereCam camera_2;
extern PlaneCam camera_3;

// Sun & light
extern float dayTime;
extern float sunAngDist;
extern Light sunLight;

#endif


