
#include<iostream>

#include <glm/gtc/type_ptr.hpp>

#include "toolkit.hpp"


double pi = 3.14159265359;


// Model Matrix -----------------------------------------------------------------

glm::mat4 modelMatrix()
{
	glm::mat4 mm = glm::mat4(1.0f);

	//mm = glm::translate(mm, glm::vec3(0.0f, 0.0f, 0.0f));
	//mm = glm::rotate(mm, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//mm = glm::scale(mm, glm::vec3(1.0f, 1.0f, 1.0f));

	return mm;
}

/// Get a model matrix depending on the provided arguments. Vector elements: X, Y, Z. Rotations specified in sexagesimal degrees, and positive if clockwise (looking from each axis).
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

/// Print the contents of a glm::mat4
void printMat4(glm::mat4 matrix)
{
	double array[16] = { 0.0 };

	const float* pSource = (const float*)glm::value_ptr(matrix);

	for (int i = 0; i < 16; ++i)
		array[i] = pSource[i];

	std::cout << array[0]  << "  " << array[1]  << "  " << array[2]  << "  " << array[3] << std::endl;
	std::cout << array[4]  << "  " << array[5]  << "  " << array[6]  << "  " << array[7] << std::endl;
	std::cout << array[8]  << "  " << array[9]  << "  " << array[10] << "  " << array[11] << std::endl;
	std::cout << array[12] << "  " << array[13] << "  " << array[14] << "  " << array[15] << std::endl;
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

size_t getPlane(std::vector<VertexPT>& vertexDestination, std::vector<uint32_t>& indicesDestination, float vertSize, float horSize)
{
	vertexDestination = std::vector<VertexPT>{
	VertexPT(glm::vec3(-horSize/2,  vertSize/2, 0.f), glm::vec2(0, 0)),
	VertexPT(glm::vec3(-horSize/2, -vertSize/2, 0.f), glm::vec2(0, 1)),
	VertexPT(glm::vec3( horSize/2, -vertSize/2, 0.f), glm::vec2(1, 1)),
	VertexPT(glm::vec3( horSize/2,  vertSize/2, 0.f), glm::vec2(1, 0)) };

	indicesDestination = std::vector<uint32_t>{ 0, 1, 3,  1, 2, 3 };

	return 4;
}

size_t getPlaneNDC(std::vector<VertexPT>& vertexDestination, std::vector<uint32_t>& indicesDestination, float vertSize, float horSize)
{
	vertexDestination = std::vector<VertexPT>{ 
		VertexPT( glm::vec3(-horSize/2, -vertSize/2, 0.f), glm::vec2(0, 0) ),
		VertexPT( glm::vec3(-horSize/2,  vertSize/2, 0.f), glm::vec2(0, 1) ),
		VertexPT( glm::vec3( horSize/2,  vertSize/2, 0.f), glm::vec2(1, 1) ),
		VertexPT( glm::vec3( horSize/2, -vertSize/2, 0.f), glm::vec2(1, 0) ) };

	indicesDestination = std::vector<uint32_t>{ 0, 1, 3,  1, 2, 3 };

	return 4;
}

// Skybox
std::vector<VertexPT> v_posx = { VertexPT(glm::vec3( 1,  1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3( 1,  1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3( 1, -1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3( 1, -1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_posy = { VertexPT(glm::vec3(-1,  1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3(-1,  1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3( 1,  1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3( 1,  1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_posz = { VertexPT(glm::vec3(-1, -1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3(-1,  1,  1), glm::vec2(0, 1)),  VertexPT(glm::vec3( 1,  1,  1), glm::vec2(1, 1)),  VertexPT(glm::vec3( 1, -1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_negx = { VertexPT(glm::vec3(-1, -1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3(-1, -1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3(-1,  1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3(-1,  1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_negy = { VertexPT(glm::vec3( 1, -1,  1), glm::vec2(0, 0)),  VertexPT(glm::vec3( 1, -1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3(-1, -1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3(-1, -1,  1), glm::vec2(1, 0)) };
std::vector<VertexPT> v_negz = { VertexPT(glm::vec3(-1,  1, -1), glm::vec2(0, 0)),  VertexPT(glm::vec3(-1, -1, -1), glm::vec2(0, 1)),  VertexPT(glm::vec3( 1, -1, -1), glm::vec2(1, 1)),  VertexPT(glm::vec3( 1,  1, -1), glm::vec2(1, 0)) };

std::vector<uint32_t> i_square = { 0, 1, 3,  1, 2, 3 };

std::vector<VertexPT> v_cube =
{
	VertexPT(glm::vec3(-1, -1,  1), glm::vec2(0., 1/3.)),
	VertexPT(glm::vec3(-1, -1, -1), glm::vec2(0., 2/3.)),
	VertexPT(glm::vec3(-1,  1,  1), glm::vec2(.25, 1/3.)),
	VertexPT(glm::vec3(-1,  1, -1), glm::vec2(.25, 2/3.)),
	VertexPT(glm::vec3( 1,  1,  1), glm::vec2(.5, 1/3.)),
	VertexPT(glm::vec3( 1,  1, -1), glm::vec2(.5, 2/3.)),
	VertexPT(glm::vec3( 1, -1,  1), glm::vec2(.75, 1/3.)),
	VertexPT(glm::vec3( 1, -1, -1), glm::vec2(.75, 2/3.)),
	VertexPT(glm::vec3(-1, -1,  1), glm::vec2(1., 1/3.)),
	VertexPT(glm::vec3(-1, -1, -1), glm::vec2(1., 2/3.)),
	VertexPT(glm::vec3(-1, -1,  1), glm::vec2(.25, 0.)),
	VertexPT(glm::vec3( 1, -1,  1), glm::vec2(.5, 0.)),
	VertexPT(glm::vec3(-1, -1, -1), glm::vec2(.25, 1.)),
	VertexPT(glm::vec3( 1, -1, -1), glm::vec2(.5, 1.))
};

std::vector<uint32_t> i_inCube = { 0, 1, 2,  1, 3, 2,  2, 3, 4,  3, 5, 4,  4, 5, 6,  5, 7, 6,  6, 7, 8,  7, 9, 8,  10, 2, 11,  2, 4, 11,  3, 12, 5,  12, 13, 5 };

bool ifOnce::ifBigger(float a, float b)
{
	if (std::find(checked.begin(), checked.end(), b) != checked.end())
		return false;
	else if (a > b)
	{
		checked.push_back(b); 
		return true;
	}
	else 
		return false;
}

glm::mat4 sunMM(glm::vec3 camPos, float dayTime, float sunDist, float sunAngDist)
{
	float sunSize = 2 * sunDist * tan(sunAngDist / 2);
	float sunAng = (dayTime - 6) * (pi / 12);
	float xRotation = 0.f;
	float yRotation = 90. + (180. - glm::degrees(sunAng));
	float zRotation = 90.f;

	return modelMatrix(
		glm::vec3(sunSize, sunSize, sunSize),
		glm::vec3(xRotation, yRotation, zRotation),
		glm::vec3(camPos.x + sunDist * cos(sunAng), camPos.y + 0.f, camPos.z + sunDist * sin(sunAng)) );
}

glm::vec3 sunLightDirection(float dayTime)
{
	glm::vec3 direction;
	//direction.x = (dayTime - 18) * (pi / 12)
	direction.x = -pi/2;
	direction.y = 0;
	direction.z = 0;

	return direction;
}