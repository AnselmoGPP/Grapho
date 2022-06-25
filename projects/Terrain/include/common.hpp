#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

#include "ubo.hpp"


// File's paths
#if defined(__unix__)
const std::string shaders_dir("../../../projects/Terrain/shaders/SPIRV/");
const std::string textures_dir("../../../textures/");
#elif _WIN64 || _WIN32
const std::string SHADERS_DIR("../../../projects/Terrain/shaders/SPIRV/");
const std::string MODELS_DIR("../../../models/");
const std::string TEXTURES_DIR("../../../textures/");
#endif

// Sun & light
extern float dayTime;
extern float sunAngDist;
extern Light sun;

#endif


