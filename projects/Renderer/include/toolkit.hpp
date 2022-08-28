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
glm::mat4 modelMatrix(const glm::vec3& scale, const glm::vec3& rotation, const glm::vec3& translation);

/// Model matrix for Normals (it's really a mat3, but a mat4 is returned because it is easier to pass to shader since mat4 is aligned with 16 bytes). Normals are passed to fragment shader in world coordinates, so they have to be multiplied by the model matrix (MM) first (this MM should not include the translation part, so we just take the upper-left 3x3 part). However, non-uniform scaling can distort normals, so we have to create a specific MM especially tailored for normal vectors: mat3(transpose(inverse(model))) * vec3(normal).
glm::mat4 modelMatrixForNormals(const glm::mat4& modelMatrix);

/// Get a mat3 from a mat4
template<typename T>
glm::mat3 toMat3(const T &matrix)
{
    //const float* pSource = (const float*)glm::value_ptr(matrix);
    glm::mat3 result;

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            result[i][j] = matrix[i][j];

    return result;
}

/// Print the contents of a glm::matX. Useful for debugging.
template<typename T>
void printMat(const T& matrix)
{
    //const float* pSource = (const float*)glm::value_ptr(matrix);
    glm::length_t length = matrix.length();
    float output;

    for (int i = 0; i < length; i++)
    {
        for (int j = 0; j < length; j++)
            std::cout << ((abs(matrix[i][j]) < 0.0001) ? 0 : matrix[i][j]) << "  ";
            //std::cout << ((abs(pSource[i * length + j]) < 0.0005) ? 0 : pSource[i * length + j]) << "  ";

        std::cout << std::endl;
    }
}

/// Print the contents of a glm::vecX. Useful for debugging
template<typename T>
void printVec(const T& vec)
{
    //const float* pSource = (const float*)glm::value_ptr(matrix);
    glm::length_t length = vec.length();
    float output;

    for (int i = 0; i < length; i++)
        std::cout << ((abs(vec[i]) < 0.0001) ? 0 : vec[i]) << "  ";

    std::cout << std::endl;
}


// Vertex sets -----------------------------------------------------------------

/// Get the XYZ axis as 3 RGB lines
size_t getAxis(std::vector<VertexPC>& vertexDestination, std::vector<uint16_t>& indicesDestination, int lengthFromCenter, float colorIntensity);

/// Get a set of lines that form a grid
size_t getGrid(std::vector<VertexPC>& vertexDestination, std::vector<uint16_t>& indicesDestination, int stepSize, size_t stepsPerSide, float height, glm::vec3 color);

/// (Local space) Get a VertexPT of a square (vertSize x horSize), its indices, and number of vertices (4). Used for draws that use MVP matrix (example: sun).
size_t getPlane(std::vector<VertexPT>& vertexDestination, std::vector<uint16_t>& indicesDestination, float vertSize, float horSize);

/// (NDC space) Get a VertexPT of a square (vertSize x horSize), its indices, and number of vertices (4). Used for draws that doesn't use MVP matrix (example: reticule).
size_t getPlaneNDC(std::vector<VertexPT>& vertexDestination, std::vector<uint16_t>& indicesDestination, float vertSize, float horSize);

/// Skybox
extern std::vector<VertexPT> v_cube;
extern std::vector<uint16_t> i_inCube;

// Others -----------------------------------------------------------------

extern double pi;

float sphereArea(float radius);

/// This class checks if argument X (float) is bigger than argument Y (float). But if it is true once, then it will be false in all the next calls. This is useful for executing something once only after X time (used for testing in graphicsUpdate()). Example: obj.ifBigger(time, 5);
class ifOnce
{
	std::vector<float> checked;

public:
	bool ifBigger(float a, float b);
};

namespace Sun
{
    /**
    @brief Get Model matrix for the sun.
        @param pos Camera position.
        @param dayTime Time of the day (12,5 = 12:30). It determines the sun angle.
        @param sunDist Sun distance from camPos.
        @param sunAngDist Sun size as angle in the celestial sphere.
    */
    glm::mat4 MM(glm::vec3 camPos, float dayTime, float sunDist, float sunAngDist);

    /// Get light direction from sun, given a day time. Used for directional and spot lights.
    glm::vec3 lightDirection(float dayTime);
};



/// Icosahedron data (vertices, colors, indices, normals)
struct Icosahedron
{
    Icosahedron(float multiplier = 1.f);

    const int numIndicesx3  = 20 * 3;
    const int numVerticesx3 = 12 * 3;
    const int numColorsx4   = 12 * 4;

    static float vertices   [12 * 3];
    static float colors     [12 * 4];
    static unsigned indices [20 * 3];
    static float normals    [12 * 3];

    static std::vector<float> icos;     // Vertices & colors (without alpha)
    static std::vector<float> index;    // Indices
};


#endif