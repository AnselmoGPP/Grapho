#ifndef DATA_HPP
#define DATA_HPP

#include <string>
#include <vector>
#include <functional>
#include "renderer.hpp"


#if defined(__unix__)
const std::string shaders_dir("../../../projects/Renderer/shaders/");
const std::string textures_dir("../../../textures/");
#elif _WIN64 || _WIN32
const std::string SHADERS_DIR("../../../projects/Renderer/shaders/");
const std::string MODELS_DIR("../../../models/");
const std::string TEXTURES_DIR("../../../textures/");
#endif


// Cottage config data --------------------

glm::mat4 cottage_MM(float time);


// Room config data --------------------

glm::mat4 room1_MM(float time);
glm::mat4 room2_MM(float time);
glm::mat4 room3_MM(float time);
glm::mat4 room4_MM(float time);
extern std::vector< std::function<glm::mat4(float)> > room_MM;


#endif