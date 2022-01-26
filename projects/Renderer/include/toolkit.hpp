#ifndef AUXILIARY_HPP
#define AUXILIARY_HPP

#include <vector>

#include "vertex.hpp"

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

/// Print content of a glm::mat4 object
void printMat4(glm::mat4 matrix);

// Vertex sets -----------------------------------------------------------------

/// Get the XYZ axis as 3 RGB lines
size_t getAxis(std::vector<VertexPC>& vertexDestination, std::vector<uint32_t>& indicesDestination, int lengthFromCenter, float colorIntensity);

/// Get a set of lines that form a grid
size_t getGrid(std::vector<VertexPC>& vertexDestination, std::vector<uint32_t>& indicesDestination, int stepSize, size_t stepsPerSide, glm::vec3 color);

/// (Local space) Get a VertexPT of a square (vertSize x horSize), its indices, and number of vertices (4). Used for draws that use MVP matrix.
size_t getPlane(std::vector<VertexPT>& vertexDestination, std::vector<uint32_t>& indicesDestination, float vertSize, float horSize);

/// (NDC space) Get a VertexPT of a square (vertSize x horSize), its indices, and number of vertices (4). Used for draws that doesn't use MVP matrix.
size_t getPlaneNDC(std::vector<VertexPT>& vertexDestination, std::vector<uint32_t>& indicesDestination, float vertSize, float horSize);

/// Skybox
extern std::vector<VertexPT> v_posx, v_posy, v_posz, v_negx, v_negy, v_negz;
extern std::vector<uint32_t> i_square;
extern std::vector<VertexPT> v_cube;
extern std::vector<uint32_t> i_inCube;

// Others -----------------------------------------------------------------

extern double pi;

/// This class checks if X argument (float) is bigger than some other argument (float). But if it is true once, then it will be false in all the next calls. This is useful for executing something once only after X time (used for testing in graphicsUpdate()). Example: obj.ifBigger(time, 5);
class ifOnce
{
	std::vector<float> checked;

public:
	bool ifBigger(float a, float b);
};

/*  @brief Get Model matrix for the sun.
        @param pos Camera position.
        @param dayTime Time of the day (12,5 = 12:30). It determines the sun angle.
        @param sunDist Sun distance from camPos.
        @param sunAngDist Sun size as angle in the celestial sphere.
*/
glm::mat4 sunMM(glm::vec3 camPos, float dayTime, float sunDist, float sunAngDist);

glm::vec3 sunLightDirection(float dayTime);

#endif