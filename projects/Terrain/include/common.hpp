#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

#include "ubo.hpp"
#include "camera.hpp"

// File's paths
extern const std::string shadersDir;
extern const std::string vertexDir;
extern const std::string texDir;

// Camera
extern FreePolarCam camera_1;
extern SphereCam camera_2;
extern PlaneCam2 camera_3;
extern PlanetFPcam camera_4;

// Sun & light
extern float dayTime;
extern float sunAngDist;
extern Light sunLight;

#endif


