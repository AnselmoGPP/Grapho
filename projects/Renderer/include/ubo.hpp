#ifndef UBO_HPP
#define UBO_HPP

#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// GLM uses OpenGL depth range [-1.0, 1.0]. This macro forces GLM to use Vulkan range [0.0, 1.0].
#define GLM_ENABLE_EXPERIMENTAL				// Required for using std::hash functions for the GLM types (since gtx folder contains experimental extensions)
#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>	// Generate transformations matrices with glm::rotate (model), glm::lookAt (view), glm::perspective (projection).
//#include <glm/gtx/hash.hpp>

#include "environment.hpp"

/*
	Content:
		- UBOtype
		- UBOdynamic (UBOset)
			- UBO_MVP
			- UBO_MVPN
*/

/// Class used for configuring the dynamic UBO. UBO attributes: Model/View/Projection matrices, Model matrix for normals, Lights.
class UBOtype
{
public:
	UBOtype(size_t numM = 0, size_t numV = 0, size_t numP = 0, size_t numMN = 0, size_t numLights = 0);	///< Constructor. Parameters: numM (model matrices), numV (view matrices), numP (projection matrices), numMN (model matrices for normals), numLights (lights).

	static const std::array<size_t, 5>	attribsSize;	///< Size of each type of attribute. attribsSize = { 64, 64, 64, 36+12, 176 } (includes padding for getting alignment with 16 bits)
	std::array<size_t, 5> numEachAttrib;				///< Number of attributes of each type. Defined by the user at construction.
};

/// Data structure for light. Sent to fragment shader.
struct Light
{
	Light();
	void turnOff();
	void setDirectional(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular);
	void setPoint(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic);
	void setSpot(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic, float cutOff, float outerCutOff);

	alignas(16) int lightType;			///< Possible values:  0: no light, 1: directional, 2: point, 3: spot
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 direction;

	alignas(16) glm::vec3 ambient;
	alignas(16) glm::vec3 diffuse;
	alignas(16) glm::vec3 specular;

	alignas(16) float constant;
	alignas(16) float linear;
	alignas(16) float quadratic;

	alignas(16) float cutOff;
	alignas(16) float outerCutOff;
};

/**
*	Structure used for storing a set of UBOs in the same structure in order to allow us to render the same model many times.
*	Attributes of a single UBO: Model, View, Projection, ModelForNormals, Lights
*	We may create a set of dynamic UBOs (count), each one containing a number of different attributes (5), each one containing 0 or more attributes of their type (numEachAttrib).
*	If count == 0, the buffer created will have size == range (not totalBytes). If range == 0, no buffer is created.
*	User should call destroyUniformBuffers() before createUniformBuffers().
*	Model matrix for Normals: Normals are passed to fragment shader in world coordinates, so they have to be multiplied by the model matrix (MM) first (this MM should not include the translation part, so we just take the upper-left 3x3 part). However, non-uniform scaling can distort normals, so we have to create a specific MM especially tailored for normal vectors.
*/
struct UBO
{
	UBO(VulkanEnvironment& e, size_t dynUBOcount, const UBOtype& uboType, VkDeviceSize minUBOffsetAlignment);	///< Constructor. Parameters: dynUBOcount (number of dynamic UBOs), uboType (defines what a single UBO contains), minUBOffsetAlignment (alignment for each UBO required by the GPU).
	~UBO() = default;

	void resize(size_t newCount);			///< Set the number of dynamic UBO in the UBO.
	void dirtyResize(size_t newCount);		///< Modifies ubo size only (useful for allowing input beyond the ubo's end in case you plan to increment size later).
	void createUniformBuffers();			///< Create uniform buffers (type of descriptors that can be bound) (VkBuffer & VkDeviceMemory), one for each swap chain image. At least one is created (if count == 0, a buffer of size "range" is created).
	void destroyUniformBuffers();			///< Destroy the uniform buffers (VkBuffer) and their memories (VkDeviceMemory).

	void setModelM(size_t posDyn, size_t attrib, const glm::mat4& matrix);	///< Assign a Model matrix (matrix) to a given dynamic UBO (posDyn) and attribute position (attrib) (a dynamic UBO can have more than one attribute of the same type).
	void setViewM (size_t posDyn, size_t attrib, const glm::mat4& matrix);	///< Assign a View matrix (matrix) to a given dynamic UBO (posDyn) and attribute position (attrib) (a dynamic UBO can have more than one attribute of the same type).
	void setProjM (size_t posDyn, size_t attrib, const glm::mat4& matrix);	///< Assign a Projection matrix (matrix) to a given dynamic UBO (posDyn) and attribute position (attrib) (a dynamic UBO can have more than one attribute of the same type).
	void setMNorm (size_t posDyn, size_t attrib, const glm::mat3& matrix);	///< Assign a Model-matrix-for-normals (matrix) to a given dynamic UBO (posDyn) and attribute position (attrib) (a dynamic UBO can have more than one attribute of the same type).
	void setLight (size_t posDyn, size_t attrib, Light& light);				///< Assign a light (light) to a given dynamic UBO (posDyn) and attribute position (attrib) (a dynamic UBO can have more than one attribute of the same type).

	size_t						count;					///< Number of dynamic UBOs
	size_t						dirtyCount;				///< Number of dynamic UBOs, including those generated with dirtyResize()
	VkDeviceSize				range;					///< Size (bytes) of an aligned dynamic UBO (example: 4)
	size_t						totalBytes;				///< Size (bytes) of the set of dynamic UBOs (example: 12)
	std::vector<uint32_t>		dynamicOffsets;			///< Offsets for each dynamic UBO
	std::array<size_t, 5>		numEachAttrib;			///< Number of attributes of each type.

	std::vector<char>			ubo;					///< Stores the UBO that will be passed to vertex shader (MVP, M for normals, light...). Its attributes are aligned to 16-byte boundary.
	std::vector<VkBuffer>		uniformBuffers;			///< Opaque handle to a buffer object (here, uniform buffer). One for each swap chain image.
	std::vector<VkDeviceMemory>	uniformBuffersMemory;	///< Opaque handle to a device memory object (here, memory for the uniform buffer). One for each swap chain image.

	friend void createBuffer(VulkanEnvironment& e, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

private:
	VulkanEnvironment& e;

	size_t getPos(size_t dynUBO, size_t attribSet, size_t attrib);	///< Get position of some attribute in UBO::ubo. Three dimensions: dynUBO (dynamic UBO), attribSet (attribute type), attrib (attribute position).
};

/// Model-View-Projection matrix as a UBO (Uniform buffer object) (https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/)
struct UBO_MVP {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};
 
struct UBO_MVPN {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::mat3 normalMatrix;
};


#endif