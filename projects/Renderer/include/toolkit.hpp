#ifndef AUXILIARY_HPP
#define AUXILIARY_HPP

#include <vector>
#include "models_2.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// GLM uses OpenGL depth range [-1.0, 1.0]. This macro forces GLM to use Vulkan range [0.0, 1.0].
//#define GLM_ENABLE_EXPERIMENTAL			// Required for using std::hash functions for the GLM types (since gtx folder contains experimental extensions)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>		// Generate transformations matrices with glm::rotate (model), glm::lookAt (view), glm::perspective (projection).
//#include <glm/gtx/hash.hpp>

// Model Matrix -----------------------------------------------------------------

/// Get a basic Model Matrix
glm::mat4 modelMatrix();

/// Get a user-defined Model Matrix 
glm::mat4 modelMatrix(glm::vec3 scale, glm::vec3 rotation, glm::vec3 translation);

// Vertex sets -----------------------------------------------------------------

size_t getAxis(std::vector<VertexPC>& vertexDestination, std::vector<uint32_t>& indicesDestination, int lengthFromCenter, float colorIntensity);

size_t getGrid(std::vector<VertexPC>& vertexDestination, std::vector<uint32_t>& indicesDestination, int stepSize, size_t stepsPerSide, glm::vec3 color);

extern std::vector<VertexPT> v_posx, v_posy, v_posz, v_negx, v_negy, v_negz;	// Skybox vertex
extern std::vector<uint32_t> i_square;											// Skybox indices

#endif