
#include "data.hpp"


std::vector< std::function<glm::mat4(float)> > room_MM{ /*room1_MM, room2_MM, room3_MM, room4_MM*/ };

std::vector<VertexPT> v_floor = {
	VertexPT(glm::vec3(-100,  100, -10), glm::vec2(0.f, 0.f)),
	VertexPT(glm::vec3(-100, -100, -10), glm::vec2(0.f, 1.f)),
	VertexPT(glm::vec3( 100, -100, -10), glm::vec2(1.f, 1.f)),
	VertexPT(glm::vec3( 100,  100, -10), glm::vec2(1.f, 0.f))
};

std::vector<uint32_t> i_floor = { 0, 1, 3,  1, 2, 3 };

std::vector<VertexPC> v_points = {
	VertexPC(glm::vec3(-10, -10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(  0, -10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3( 10, -10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(-10,   0,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(  0,   0,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3( 10,   0,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(-10,  10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(  0,  10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3( 10,  10,  10), glm::vec3(1.f, 0.f, 0.f))
};

