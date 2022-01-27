#ifndef DATA_HPP
#define DATA_HPP

#include <string>
#include <vector>
#include <functional>

#include "vertex.hpp"


#if defined(__unix__)
const std::string shaders_dir("../../../projects/Renderer/shaders/SPIRV/");
const std::string textures_dir("../../../textures/");
#elif _WIN64 || _WIN32
const std::string SHADERS_DIR("../../../projects/Renderer/shaders/SPIRV/");
const std::string MODELS_DIR("../../../models/");
const std::string TEXTURES_DIR("../../../textures/");
#endif


extern std::vector< std::function<glm::mat4(float)> > room_MM;	// Store callbacks of type 'glm::mat4 callb(float a)'

// Others

extern std::vector<VertexPT>	v_floor;	// Vertex, color, texture coordinates
extern std::vector<uint32_t>	i_floor;	// Indices

extern std::vector<VertexPC>	v_points;

#endif