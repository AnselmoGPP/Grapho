
#include "toolkit.hpp"

// Model Matrix -----------------------------------------------------------------

glm::mat4 modelMatrix()
{
	glm::mat4 mm = glm::mat4(1.0f);

	mm = glm::translate(mm, glm::vec3(0.0f, 0.0f, 0.0f));
	//mm = glm::rotate(mm, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//mm = glm::scale(mm, glm::vec3(1.0f, 1.0f, 1.0f));

	return mm;
}

/// Get a model matrix depending on the provided arguments. Vector elements: X, Y, Z. Rotations specified in sexagesimal degrees.
glm::mat4 modelMatrix(glm::vec3 scale, glm::vec3 rotation, glm::vec3 translation)
{
	glm::mat4 mm = glm::mat4(1.0f);

	mm = glm::translate(mm, translation);
	mm = glm::rotate(mm, glm::radians(rotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
	mm = glm::rotate(mm, glm::radians(rotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
	mm = glm::rotate(mm, glm::radians(rotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));
	mm = glm::scale(mm, scale);

	return mm;
}

// Vertex sets -----------------------------------------------------------------

size_t getAxis(std::vector<VertexPC>& vertexDestination, std::vector<uint32_t>& indicesDestination, int lengthFromCenter, float colorIntensity)
{
	// Vertex buffer
	vertexDestination = std::vector<VertexPC> { 
		VertexPC(glm::vec3( 0, 0, 0), glm::vec3(colorIntensity, 0, 0)),
		VertexPC(glm::vec3( lengthFromCenter, 0, 0), glm::vec3(colorIntensity, 0, 0)),
		VertexPC(glm::vec3( 0, 0, 0), glm::vec3(0, colorIntensity, 0)),
		VertexPC(glm::vec3( 0, lengthFromCenter, 0), glm::vec3(0, colorIntensity, 0)),
		VertexPC(glm::vec3( 0, 0, 0), glm::vec3(0, 0, colorIntensity)),
		VertexPC(glm::vec3( 0, 0, lengthFromCenter), glm::vec3(0, 0, colorIntensity)) };

	// Indices buffer
	indicesDestination = std::vector<uint32_t>{ 0, 1,  2, 3,  4, 5 };

	// Total number of vertex
	return 6;
}

size_t getGrid(std::vector<VertexPC>& vertexDestination, std::vector<uint32_t>& indicesDestination, int stepSize, size_t stepsPerSide, glm::vec3 color)
{
	// Vertex buffer
	int start = (stepSize * stepsPerSide) / 2;

	for (int i = 0; i <= stepsPerSide; i++)
	{
		vertexDestination.push_back({ glm::vec3(-start, -start + i * stepSize, 0), color });
		vertexDestination.push_back({ glm::vec3( start, -start + i * stepSize, 0), color });
	}

	for (int i = 0; i <= stepsPerSide; i++)
	{
		vertexDestination.push_back({ glm::vec3(-start + i * stepSize, -start, 0), color });
		vertexDestination.push_back({ glm::vec3(-start + i * stepSize,  start, 0), color });
	}
	
	// Indices buffer
	size_t numVertex = (stepsPerSide + 1) * 4;

	for (uint32_t i = 0; i < numVertex; i++)
		indicesDestination.push_back(i);

	// Total number of vertex
	return numVertex;
}

std::vector<VertexPT> v_posx = { VertexPT(glm::vec3( 1,  1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3( 1,  1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3( 1, -1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3( 1, -1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_posy = { VertexPT(glm::vec3(-1,  1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3(-1,  1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3( 1,  1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3( 1,  1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_posz = { VertexPT(glm::vec3(-1, -1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3(-1,  1,  1), glm::vec2(0, 1)),  VertexPT(glm::vec3( 1,  1,  1), glm::vec2(1, 1)),  VertexPT(glm::vec3( 1, -1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_negx = { VertexPT(glm::vec3(-1, -1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3(-1, -1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3(-1,  1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3(-1,  1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_negy = { VertexPT(glm::vec3( 1, -1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3( 1, -1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3(-1, -1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3(-1, -1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_negz = { VertexPT(glm::vec3(-1,  1, -1), glm::vec2(0, 0)),  VertexPT(glm::vec3(-1, -1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3( 1, -1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3( 1,  1, -1), glm::vec2(1, 0)) };

std::vector<uint32_t> i_square = { 0, 1, 3,  1, 2, 3 };