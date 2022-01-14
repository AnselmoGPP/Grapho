#ifndef MODELS2_HPP
#define MODELS2_HPP

#include <array>

//#include <vulkan/vulkan.h>		// From LunarG SDK. Used for off-screen rendering
#define GLFW_INCLUDE_VULKAN			// Makes GLFW load the Vulkan header with it
#include "GLFW/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// GLM uses OpenGL depth range [-1.0, 1.0]. This macro forces GLM to use Vulkan range [0.0, 1.0].
#define GLM_ENABLE_EXPERIMENTAL				// Required for using std::hash functions for the GLM types (since gtx folder contains experimental extensions)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>		// Generate transformations matrices with glm::rotate (model), glm::lookAt (view), glm::perspective (projection).
#include <glm/gtx/hash.hpp>

#include "environment.hpp"

/*
	Content:
		- VertexType
		- VertexSet
			- VertexPCT
			- VertexPC
			- VertexPT
			- VertexPTN
		- UBOtype
		- UBOdynamic (UBOset)
			- UBO_MVP
			- UBO_MVPN
		- Texture

	Data:
		- Vertex:
			- Position
			- Color
			- Texture coordinates
			- Normal
		- Descriptors:
			- UBOs
				- Model matrix
				- View matrix
				- Proyection matrix
				- Model matrix for normals
			- Textures
				- Diffuse map
				- Especular map
*/

/// Function used as friend in modelData, UBO and Texture
void createBuffer(VulkanEnvironment& e, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

/// Used for configuring VertexSet. Defines a type of vertex that may contain: Position, Color, Texture coordinates, Normal
class VertexType
{
public:
	VertexType(size_t numP, size_t numC, size_t numT, size_t numN = 0);
	~VertexType();
	VertexType& operator=(const VertexType& obj);

	VkVertexInputBindingDescription getBindingDescription();
	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
	
	const std::array<size_t, 4> attribsSize = { sizeof(glm::vec3), sizeof(glm::vec3), sizeof(glm::vec2), sizeof(glm::vec3) };

	//char* vertex;							// Vertex data
	size_t vertexSize;						// LOOK necessary? Size in bytes of a vertex object (including padding)
	std::array<size_t, 4> numEachAttrib;	// Amount of each type of attribute
};

/// VertexSet serves as a container for any object type, similarly to a std::vector, but storing such objects directly in bytes (char array). This allows ModelData objects store different Vertex types in a clean way (otherwise, templates and inheritance would be required, but code would be less clean).
class VertexSet
{
public:
	VertexSet(VertexType vertexType);
	VertexSet(VertexType vertexType, size_t numOfVertex, const void* buffer);
	~VertexSet();
	VertexSet& operator=(const VertexSet& obj);	// Not used

	VertexType Vtype;						// Vertex type. Contains the configuration of a vertex

	//VkVertexInputBindingDescription					getBindingDescription();	///< Describes at which rate to load data from memory throughout the vertices (number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance).
	//std::vector<VkVertexInputAttributeDescription>	getAttributeDescriptions();	///< Describe how to extract a vertex attribute from a chunk of vertex data originiating from a binding description. Two attributes here: position and color.

	//bool operator==(const VertexPCT& other) const;							///< Overriding of operator ==. Required for doing comparisons in loadModel().

	size_t totalBytes() const;
	size_t size() const;
	char* data() const;
	void push_back(const void* element);
	void* getElement(size_t i) const;		///< For debugging purposes

private:
	char* buffer;			// Set of vertex objects stored directly in bytes
	size_t capacity;		// (resizable) Maximum number of vertex objects that fit in buffer
	size_t numVertex;		// Number of vertex objects stored in buffer
};

/// Class used for configuring the dynamic UBO. UBO attributes: Model/View/Projection matrices, Model matrix for normals, Lights.
class UBOtype
{
public:
	UBOtype(size_t numM = 0, size_t numV = 0, size_t numP = 0, size_t numMN = 0, size_t numLights = 0);

	std::array<size_t, 5> numEachAttrib;
};

/// Data structure for light. Sent to fragment shader.
struct Light
{
	Light();
	void turnOff();
	void setDirectional(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular);
	void setPoint(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic);
	void setSpot(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic, float cutOff, float outerCutOff);

	alignas(16) int lightType;		// 0: no light, 1: directional, 2: point, 3: spot
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

/// Structure used for storing many UBOs in the same structure in order to allow us to render the same model many times.
/// Attributes of the UBO: Model, View, Projection, ModelForNormals, Lights
/// We may create a set of dynamic UBOs (dynUBOcount -> count), each one containing a number of descriptors (uboType -> numEachAttrib) (0 or more of each descriptor).
/// Uniform buffers can be created or destroyed (createUniformBuffers, destroyUniformBuffers) only if range > 0. If count == 0, the buffer created will have size == range (not totalBytes). 
/// User should call destroyUniformBuffers before createUniformBuffers. Renderer 
/// Model matrix for Normals: Normals are passed to fragment shader in world coordinates, so they have to be multiplied by the model matrix (MM) first (this MM should not include the translation part, so we just take the upper-left 3x3 part). However, non-uniform scaling can distort normals, so we have to create a specific MM especially tailored for normal vectors.
struct UBO
{
	UBO(VulkanEnvironment& e, size_t dynUBOcount, const UBOtype& uboType, VkDeviceSize minUBOffsetAlignment);
	//UBO& operator = (const UBO& obj);
	~UBO() = default;

	void resize(size_t newCount);
	void dirtyResize(size_t newCount);		///< Modifies ubo size only (useful for allowing input beyond the ubo's end in case you plan to increment size later).
	void createUniformBuffers();
	void destroyUniformBuffers();

	void setModelM(size_t posDyn, size_t attrib, const glm::mat4& matrix);
	void setViewM (size_t posDyn, size_t attrib, const glm::mat4& matrix);
	void setProjM (size_t posDyn, size_t attrib, const glm::mat4& matrix);
	void setMNorm (size_t posDyn, size_t attrib, const glm::mat3& matrix);
	void setLight (size_t posDyn, size_t attrib, Light& light);

	size_t					count;			///< Number of dynamic UBOs
	size_t					dirtyCount;		///< Number of dynamic UBOs, including those generated with dirtyResize()
	VkDeviceSize			range;			///< Size (bytes) of an aligned dynamic UBO (example: 4)
	size_t					totalBytes;		///< Size (bytes) of the set of dynamic UBOs (example: 12)
	std::vector<uint32_t>	dynamicOffsets;	///< Offsets for each dynamic UBO
	std::array<size_t, 5>	numEachAttrib;

	std::vector<char>			ubo;					///< Stores the UBO that will be passed to vertex shader (MVP, M for normals...).
	std::vector<VkBuffer>		uniformBuffers;		///< Opaque handle to a buffer object (here, uniform buffer). One for each swap chain image.
	std::vector<VkDeviceMemory>	uniformBuffersMemory;	///< Opaque handle to a device memory object (here, memory for the uniform buffer). One for each swap chain image.

	friend void createBuffer(VulkanEnvironment& e, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

private:
	VulkanEnvironment& e;
	const std::array<size_t, 5>	attribsSize = { sizeof(glm::mat4), sizeof(glm::mat4), sizeof(glm::mat4), sizeof(glm::mat3) + 12, sizeof(Light) };	// 64, 64, 64, 36+12, 176 (including padding for getting alignment with 16 bits)

	size_t getPos(size_t dynUBO, size_t attribSet, size_t attrib);
};


#include "environment.hpp"

class Texture
{
	const char* path;
	VulkanEnvironment* e;

	void createTextureImage();			///< Load an image and upload it into a Vulkan object.
	void createTextureImageView();		///< Create an image view for the texture (images are accessed through image views rather than directly).
	void createTextureSampler();		///< Create a sampler for the textures (it applies filtering and transformations).

	void copyCString(const char*& destination, const char* source);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	friend void createBuffer(VulkanEnvironment& e, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);	///< Helper function for creating a buffer (VkBuffer and VkDeviceMemory).

public:
	Texture(const char* path);
	Texture(const Texture& obj);
	~Texture();

	void loadAndCreateTexture(VulkanEnvironment& e);


	uint32_t					 mipLevels;				///< Number of levels (mipmaps)
	VkImage						 textureImage;			///< Opaque handle to an image object.
	VkDeviceMemory				 textureImageMemory;	///< Opaque handle to a device memory object.
	VkImageView					 textureImageView;		///< Image view for the texture image (images are accessed through image views rather than directly).
	VkSampler					 textureSampler;		///< Opaque handle to a sampler object (it applies filtering and transformations to a texture). It is a distinct object that provides an interface to extract colors from a texture. It can be applied to any image you want (1D, 2D or 3D).
};


/// Vertex structure containing Position, Color and Texture coordinates.
struct VertexPCT
{
	VertexPCT() = default;
	VertexPCT(glm::vec3 vertex, glm::vec3 vertexColor, glm::vec2 textureCoordinates);

	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription					getBindingDescription();	///< Describes at which rate to load data from memory throughout the vertices (number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance).
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();	///< Describe how to extract a vertex attribute from a chunk of vertex data originiating from a binding description. Two attributes here: position and color.
	bool operator==(const VertexPCT& other) const;										///< Overriding of operator ==. Required for doing comparisons in loadModel().
};

/// Hash function for VertexPCT. Implemented by specifying a template specialization for std::hash<T> (https://en.cppreference.com/w/cpp/utility/hash). Required for doing comparisons in loadModel().
template<> struct std::hash<VertexPCT> {
	size_t operator()(VertexPCT const& vertex) const;
};

/// Vertex structure containing Position and Color (useful for lines or points)
struct VertexPC
{
	VertexPC() = default;
	VertexPC(glm::vec3 vertex, glm::vec3 vertexColor);

	glm::vec3 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription					getBindingDescription();	///< Describes at which rate to load data from memory throughout the vertices (number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance).
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();	///< Describe how to extract a vertex attribute from a chunk of vertex data originiating from a binding description. Two attributes here: position and color.
	bool operator==(const VertexPC& other) const;										///< Overriding of operator ==. Required for doing comparisons in loadModel().
};

/// Hash function for VertexPCT. Implemented by specifying a template specialization for std::hash<T> (https://en.cppreference.com/w/cpp/utility/hash). Required for doing comparisons in loadModel().
template<> struct std::hash<VertexPC> {
	size_t operator()(VertexPC const& vertex) const;
};

/// Vertex structure containing Position and Texture
struct VertexPT
{
	VertexPT() = default;
	VertexPT(glm::vec3 vertex, glm::vec2 textureCoordinates);

	glm::vec3 pos;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription					getBindingDescription();	///< Describes at which rate to load data from memory throughout the vertices (number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance).
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();	///< Describe how to extract a vertex attribute from a chunk of vertex data originiating from a binding description. Two attributes here: position and color.
	bool operator==(const VertexPT& other) const;										///< Overriding of operator ==. Required for doing comparisons in loadModel().
};

/// Hash function for VertexPT. Implemented by specifying a template specialization for std::hash<T> (https://en.cppreference.com/w/cpp/utility/hash). Required for doing comparisons in loadModel().
template<> struct std::hash<VertexPT> {
	size_t operator()(VertexPT const& vertex) const;
};

/// Vertex structure containing Position, Texture, and Normals
struct VertexPTN
{
	glm::vec3 pos;
	glm::vec2 texCoord;
	glm::vec3 normals;
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