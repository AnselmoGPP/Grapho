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
	UBO memory organization in the GPU: 
		- The UBO has to be aligned with "minUBOffsetAlignment" bytes.
		- The variables or members of structs that you pass to the shader have to be aligned with 16 bytes (but variables or struct members created inside the shader doesn't).
		- Due to the 16-bytes alignment requirement, you should pass variables that fit 16 bytes (example: vec4, float[4], int[4]...) or fit your variables in packages of 16 bytes (example: float + int + vec2).

|--------------------------------minUBOffsetAlignment(256)-----------------------------|

|---------16---------||---------16---------||---------16---------||---------16---------|

|----------------------------my struct---------------------------||--int--|

|-float-|             |----vec3----|        |--------vec4--------|

*/

extern size_t UniformAlignment;	// Alignment required for each uniform in the UBO (usually, 16 bytes).
extern size_t MMsize;			// Model matrix
extern size_t VMsize;			// View matrix
extern size_t PMsize;			// Proyection matrix
extern size_t MMNsize;			// Model matrix for Normals
extern size_t lightSize;		// Light
extern size_t vec4size;			// glm::vec4
extern size_t materialSize;		// Material

/// Class used for configuring the dynamic UBO. UBO attributes: Model/View/Projection matrices, Model matrix for normals, Lights.
struct UBOconfig
{
	UBOconfig(size_t dynBlocks = 0, size_t size1 = 0, size_t size2 = 0, size_t size3 = 0, size_t size4 = 0, size_t size5 = 0);

	size_t dynBlocksCount;
	std::vector<size_t> attribsSize;
};

/**
	@struct Light
	@brief Data structure for light. Sent to fragment shader.

	Maybe their members should be 16-bytes aligned (alignas(16)).
	Usual light values:
	<ul>
		<li>Ambient: Low value</li>
		<li>Diffuse: Exact color of the light</li>
		<li>Specular: Full intensity: vec3(1.0)</li>
	</ul>
*/
struct Light
{
	Light();
	void turnOff();
	void setDirectional(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular);
	void setPoint(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic);
	void setSpot(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic, float cutOff, float outerCutOff);

	alignas(16) int lightType;			//!< 0: no light, 1: directional, 2: point, 3: spot

	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 direction;

	alignas(16) glm::vec3 ambient;
	alignas(16) glm::vec3 diffuse;
	alignas(16) glm::vec3 specular;

	alignas(16) glm::vec3 degree;		//!< vec3( constant, linear, quadratic )
	alignas(16) glm::vec2 cutOff;		//!< vec2( cutOff, outerCutOff )
};

/**
	@struct Material
	@brief Data structure for a material. Passed to shader as UBO. No textures, just values for Diffuse, Specular & Shininess.

	Basic values:
	<ul>
		<li>Ambient (not included): Object color</li>
		<li>Diffuse (albedo): Object color</li>
		<li>Specular: Specular map</li>
		<li>Shininess: Object shininess</li>
	</ul>
	Examples: http://devernay.free.fr/cours/opengl/materials.html
*/
struct Material
{
	Material(glm::vec3& diffuse, glm::vec3& specular, float shininess);

	// alignas(16) vec3 ambient;		// Already controlled with the light
	alignas(16) glm::vec3 diffuse;		// or sampler2D diffuseT;
	alignas(16) glm::vec3 specular;		// or sampler2D specularT;
	alignas(16) float shininess;
};


/**
*	@struct UBO
*	@brief Structure used for storing a set of UBOs in the same structure (many UBOs can be used for rendering the same model many times).
*	
*	Attributes of a single UBO: Model, View, Projection, ModelForNormals, Lights
*	We may create a set of dynamic UBOs (count), each one containing a number of different attributes (5), each one containing 0 or more attributes of their type (numEachAttrib).
*	If count == 0, the buffer created will have size == range (instead of totalBytes, which is == 0). If range == 0, no buffer is created.
*	User should call destroyUniformBuffers() before createUniformBuffers().
*	Model matrix for Normals: Normals are passed to fragment shader in world coordinates, so they have to be multiplied by the model matrix (MM) first (this MM should not include the translation part, so we just take the upper-left 3x3 part). However, non-uniform scaling can distort normals, so we have to create a specific MM especially tailored for normal vectors: mat3(transpose(inverse(model))) * aNormal.
*/
struct UBO
{
	UBO(VulkanEnvironment& e, const UBOconfig& config, VkDeviceSize minUBOffsetAlignment);	//!< Constructor. Parameters: dynUBOcount (number of dynamic UBOs), uboType (defines what a single UBO contains), minUBOffsetAlignment (alignment for each UBO required by the GPU).
	~UBO() = default;

	template<typename T>
	void setUniform(size_t dynBlock, size_t uniform, T &newValue, size_t offset = 0);
	void resize(size_t newDynBlocksCount);				//!< Set the number of dynamic UBO in the UBO.
	void hiddenResize(size_t newDynBlocksCount);		//!< Modifies ubo size only (useful for allowing input beyond the ubo's end in case you plan to increment size later) (see Renderer::setRenders()).

	void createUniformBuffers();						//!< Create uniform buffers (type of descriptors that can be bound) (VkBuffer & VkDeviceMemory), one for each swap chain image. At least one is created (if count == 0, a buffer of size "range" is created).
	void destroyUniformBuffers();						//!< Destroy the uniform buffers (VkBuffer) and their memories (VkDeviceMemory).

	size_t						dynBlocksCount;			//!< Number of dynamic UBOs
	size_t						hiddenCount;			//!< Actual number of dynamic UBOs, including those generated with hiddenResize().

	VkDeviceSize				range;					//!< Size (bytes) of an aligned dynamic UBO (example: 4) (at least, minUBOffsetAlignment)
	size_t						totalBytes;				//!< Size (bytes) of the set of dynamic UBOs (example: 12)

	std::vector<size_t>			attribsSize;
	std::vector<uint32_t>		dynamicOffsets;			//!< Offsets for each dynamic UBO

	std::vector<uint8_t>		ubo;					//!< Stores the UBO that will be passed to vertex shader (MVP, M for normals, light...). Its attributes are aligned to 16-byte boundary.
	std::vector<VkBuffer>		uniformBuffers;			//!< Opaque handle to a buffer object (here, uniform buffer). One for each swap chain image.
	std::vector<VkDeviceMemory>	uniformBuffersMemory;	//!< Opaque handle to a device memory object (here, memory for the uniform buffer). One for each swap chain image.

private:
	VulkanEnvironment& e;

	std::vector<size_t> uniformsOffsets;
};


template<typename T>
void UBO::setUniform(size_t dynBlock, size_t uniform, T& newValue, size_t offset)
{
	uint8_t* destination = ubo.data() + (dynBlock * range + uniformsOffsets[uniform] + offset);
	memcpy(destination, &newValue, sizeof(T));
}

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