
#include<iostream>

#include <glm/gtc/type_ptr.hpp>

#include "toolkit.hpp"


double pi = 3.141592653589793238462;

float sphereArea(float radius) { return 4 * pi * radius * radius; }

glm::vec3 reflect(glm::vec3 lightRay, glm::vec3 normal)
{
	//normal = glm::normalize(normal);
	return lightRay - 2 * glm::dot(lightRay, normal) * normal;
}

// Model Matrix -----------------------------------------------------------------

glm::mat4 modelMatrix() { return glm::mat4(1.0f); }

/// Get a model matrix depending on the provided arguments. Vector elements: X, Y, Z. Rotations specified in sexagesimal degrees, and positive if clockwise (looking from each axis).
glm::mat4 modelMatrix(const glm::vec3& scale, const glm::vec3& rotation, const glm::vec3& translation)
{
	glm::mat4 mm(1.0f);

	mm = glm::translate(mm, translation);
	mm = glm::rotate(mm, glm::radians(rotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
	mm = glm::rotate(mm, glm::radians(rotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
	mm = glm::rotate(mm, glm::radians(rotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));
	mm = glm::scale(mm, scale);

	return mm;
}

glm::mat4 modelMatrixForNormals(const glm::mat4& modelMatrix)
{
	//return toMat3(glm::transpose(glm::inverse(modelMatrix)));
	return glm::transpose(glm::inverse(modelMatrix));
}


// Vertex sets -----------------------------------------------------------------

size_t getAxis(std::vector<VertexPC>& vertexDestination, std::vector<uint16_t>& indicesDestination, int lengthFromCenter, float colorIntensity)
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
	indicesDestination = std::vector<uint16_t>{ 0, 1,  2, 3,  4, 5 };

	// Total number of vertex
	return 6;
}

size_t getGrid(std::vector<VertexPC>& vertexDestination, std::vector<uint16_t>& indicesDestination, int stepSize, size_t stepsPerSide, float height, glm::vec3 color)
{
	// Vertex buffer
	int start = (stepSize * stepsPerSide) / 2;

	for (int i = 0; i <= stepsPerSide; i++)
	{
		vertexDestination.push_back({ glm::vec3(-start, -start + i * stepSize, height), color });
		vertexDestination.push_back({ glm::vec3( start, -start + i * stepSize, height), color });
	}

	for (int i = 0; i <= stepsPerSide; i++)
	{
		vertexDestination.push_back({ glm::vec3(-start + i * stepSize, -start, height), color });
		vertexDestination.push_back({ glm::vec3(-start + i * stepSize,  start, height), color });
	}
	
	// Indices buffer
	size_t numVertex = (stepsPerSide + 1) * 4;

	for (uint16_t i = 0; i < numVertex; i++)
		indicesDestination.push_back(i);

	// Total number of vertex
	return numVertex;
}

size_t getPlane(std::vector<VertexPT>& vertexDestination, std::vector<uint16_t>& indicesDestination, float vertSize, float horSize, float height)
{
	vertexDestination = std::vector<VertexPT>{
	VertexPT(glm::vec3(-horSize/2,  vertSize/2, height), glm::vec2(0, 0)),		// top left
	VertexPT(glm::vec3(-horSize/2, -vertSize/2, height), glm::vec2(0, 1)),		// low left
	VertexPT(glm::vec3( horSize/2, -vertSize/2, height), glm::vec2(1, 1)),		// low right
	VertexPT(glm::vec3( horSize/2,  vertSize/2, height), glm::vec2(1, 0)) };	// top right

	indicesDestination = std::vector<uint16_t>{ 0, 1, 3,  1, 2, 3 };

	return 4;
}

size_t getPlaneNDC(std::vector<VertexPT>& vertexDestination, std::vector<uint16_t>& indicesDestination, float vertSize, float horSize)
{
	vertexDestination = std::vector<VertexPT>{ 
		VertexPT( glm::vec3(-horSize/2, -vertSize/2, 0.f), glm::vec2(0, 0) ),
		VertexPT( glm::vec3(-horSize/2,  vertSize/2, 0.f), glm::vec2(0, 1) ),
		VertexPT( glm::vec3( horSize/2,  vertSize/2, 0.f), glm::vec2(1, 1) ),
		VertexPT( glm::vec3( horSize/2, -vertSize/2, 0.f), glm::vec2(1, 0) ) };

	indicesDestination = std::vector<uint16_t>{ 0, 1, 3,  1, 2, 3 };

	return 4;
}

// Skybox
std::vector<VertexPT> v_cube =
{
	VertexPT(glm::vec3(-1, -1,  1), glm::vec2( 0., 1/3.)), 
	VertexPT(glm::vec3(-1, -1, -1), glm::vec2( 0., 2/3.)), 
	VertexPT(glm::vec3(-1,  1,  1), glm::vec2(.25, 1/3.)),
	VertexPT(glm::vec3(-1,  1, -1), glm::vec2(.25, 2/3.)),
	VertexPT(glm::vec3( 1,  1,  1), glm::vec2( .5, 1/3.)), 
	VertexPT(glm::vec3( 1,  1, -1), glm::vec2( .5, 2/3.)), 
	VertexPT(glm::vec3( 1, -1,  1), glm::vec2(.75, 1/3.)),
	VertexPT(glm::vec3( 1, -1, -1), glm::vec2(.75, 2/3.)),
	VertexPT(glm::vec3(-1, -1,  1), glm::vec2( 1., 1/3.)), 
	VertexPT(glm::vec3(-1, -1, -1), glm::vec2( 1., 2/3.)), 
	VertexPT(glm::vec3(-1, -1,  1), glm::vec2(.25, 0.  )),  
	VertexPT(glm::vec3( 1, -1,  1), glm::vec2( .5, 0.  )),   
	VertexPT(glm::vec3(-1, -1, -1), glm::vec2(.25, 1.  )),  
	VertexPT(glm::vec3( 1, -1, -1), glm::vec2( .5, 1.  ))    
};

std::vector<uint16_t> i_inCube = { 0, 1, 2,  1, 3, 2,  2, 3, 4,  3, 5, 4,  4, 5, 6,  5, 7, 6,  6, 7, 8,  7, 9, 8,  10, 2, 11,  2, 4, 11,  3, 12, 5,  12, 13, 5 };

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

namespace Sun
{
	/// Get Model Matrix (MM) for the sun clipboard.
	glm::mat4 MM(glm::vec3 camPos, float dayTime, float sunDist, float sunAngDist)
	{
		float sunSize = 2 * sunDist * tan(sunAngDist / 2);
		float sunAng = (dayTime - 6) * (pi / 12);

		return modelMatrix(
			glm::vec3(sunSize, sunSize, sunSize),
			glm::vec3(0.f, -90.f - glm::degrees(sunAng), 0.f),
			glm::vec3(camPos.x + sunDist * cos(sunAng), camPos.y, camPos.z + sunDist * sin(sunAng)));
	}

	/// Get direction from where the light comes from.
	glm::vec3 lightDirection(float dayTime)
	{
		float angle = (dayTime - 6) * (pi / 12);
		glm::vec3 direction;
		direction.x = cos(angle);
		direction.y = 0.f;
		direction.z = sin(angle);

		return glm::normalize(-direction);
	}
}

Icosahedron::Icosahedron(float multiplier)
{
	for (size_t i = 0; i < 12; i++)
	{
		icos[i * 6 + 0] *= multiplier;
		icos[i * 6 + 1] *= multiplier;
		icos[i * 6 + 2] *= multiplier;
	}
}

float Icosahedron::vertices[12 * 3] =
{
	 0.f,       -0.525731f,  0.850651f,
	 0.850651f,  0.f,        0.525731f,
	 0.850651f,  0.f,       -0.525731f,
	-0.850651f,  0.f,       -0.525731f,
	-0.850651f,  0.f,        0.525731f,
	-0.525731f,  0.850651f,  0.f,
	 0.525731f,  0.850651f,  0.f,
	 0.525731f, -0.850651f,  0.f,
	-0.525731f, -0.850651f,  0.f,
	 0.f,       -0.525731f, -0.850651f,
	 0.f,        0.525731f, -0.850651f,
	 0.f,        0.525731f,  0.850651f
};

float Icosahedron::colors[12 * 4] =
{
	1.0, 0.0, 0.0, 1.0,
	1.0, 0.5, 0.0, 1.0,
	1.0, 1.0, 0.0, 1.0,
	0.5, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.5, 1.0,
	0.0, 1.0, 1.0, 1.0,
	0.0, 0.5, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	0.5, 0.0, 1.0, 1.0,
	1.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.5, 1.0
};

unsigned indices[20 * 3] =
{
	1,  2,  6,
	1,  7,  2,
	3,  4,  5,
	4,  3,  8,
	6,  5,  11,
	5,  6,  10,
	9,  10, 2,
	10, 9,  3,
	7,  8,  9,
	8,  7,  0,
	11, 0,  1,
	0,  11, 4,
	6,  2,  10,
	1,  6,  11,
	3,  5,  10,
	5,  4,  11,
	2,  7,  9,
	7,  1,  0,
	3,  9,  8,
	4,  8,  0
};

float normals[12 * 3] =
{
	 0.000000f, -0.417775f,  0.675974f,
	 0.675973f,  0.000000f,  0.417775f,
	 0.675973f, -0.000000f, -0.417775f,
	-0.675973f,  0.000000f, -0.417775f,
	-0.675973f, -0.000000f,  0.417775f,
	-0.417775f,  0.675974f,  0.000000f,
	 0.417775f,  0.675973f, -0.000000f,
	 0.417775f, -0.675974f,  0.000000f,
	-0.417775f, -0.675974f,  0.000000f,
	 0.000000f, -0.417775f, -0.675973f,
	 0.000000f,  0.417775f, -0.675974f,
	 0.000000f,  0.417775f,  0.675973f
};

std::vector<float> Icosahedron::icos =
{
	 0.f,       -0.525731f,  0.850651f, 1.0, 0.0, 0.0,
	 0.850651f,  0.f,        0.525731f, 1.0, 0.5, 0.0,
	 0.850651f,  0.f,       -0.525731f, 1.0, 1.0, 0.0,
	-0.850651f,  0.f,       -0.525731f, 0.5, 1.0, 0.0,
	-0.850651f,  0.f,        0.525731f, 0.0, 1.0, 0.0,
	-0.525731f,  0.850651f,  0.f,	    0.0, 1.0, 0.5,
	 0.525731f,  0.850651f,  0.f,	    0.0, 1.0, 1.0,
	 0.525731f, -0.850651f,  0.f,	    0.0, 0.5, 1.0,
	-0.525731f, -0.850651f,  0.f,	    0.0, 0.0, 1.0,
	 0.f,       -0.525731f, -0.850651f, 0.5, 0.0, 1.0,
	 0.f,        0.525731f, -0.850651f, 1.0, 0.0, 1.0,
	 0.f,        0.525731f,  0.850651f, 1.0, 0.0, 0.5
};

std::vector<float> Icosahedron::index =
{
	1,  2,  6,
	1,  7,  2,
	3,  4,  5,
	4,  3,  8,
	6,  5,  11,
	5,  6,  10,
	9,  10, 2,
	10, 9,  3,
	7,  8,  9,
	8,  7,  0,
	11, 0,  1,
	0,  11, 4,
	6,  2,  10,
	1,  6,  11,
	3,  5,  10,
	5,  4,  11,
	2,  7,  9,
	7,  1,  0,
	3,  9,  8,
	4,  8,  0
};