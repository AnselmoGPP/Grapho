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

/*
		- VertexType
		- VertexSet
			- VertexPCT
			- VertexPC
			- VertexPT
			- VertexPTN
		- UBOdynamic (UBOset)
			- UBO_MVP
			- UBO_MVPN
*/


enum vertexSize;	// PCT, PC, PT, PTN
enum uboSize;		// MVP, MVPN

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

enum vertexSize { PCT = sizeof(VertexPCT), PC = sizeof(VertexPC), PT = sizeof(VertexPT), PTN = sizeof(VertexPTN) };


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

enum uboSize { MVP = sizeof(UBO_MVP), MVPN = sizeof(UBO_MVPN) };

/// Class used for configuring the dynamic UBO. UBO attributes: MVP matrices (MVP), Model matrix for normals (MN).
class UBOtype
{
public:
	UBOtype(size_t numM = 0, size_t numV = 0, size_t numP = 0, size_t numMN = 0);

	std::array<size_t, 4> numEachAttrib;
};

/// Structure used for storing many UBOs in the same structure in order to allow us to render the same model many times.
/// Attributes of the UBO: Model, View, Projection, ModelForNormals
struct UBOdynamic
{
	UBOdynamic(size_t UBOcount, const UBOtype& uboType, VkDeviceSize minUBOffsetAlignment);
	UBOdynamic& operator = (const UBOdynamic& obj);
	~UBOdynamic() = default;

	void resize(size_t newCount);
	void setModel(size_t position, const glm::mat4& matrix);
	void setView(size_t position, const glm::mat4& matrix);
	void setProj(size_t position, const glm::mat4& matrix);
	void setMNor(size_t position, const glm::mat3& matrix);

	size_t					count;			///< Number of dynamic UBOs
	VkDeviceSize			range;			///< Size (bytes) of an aligned dynamic UBO (example: 4)
	size_t					totalBytes;		///< Size (bytes) of the set of dynamic UBOs (example: 12)
	std::vector<uint32_t>	dynamicOffsets;	///< Offsets for each dynamic UBO
	std::array<size_t, 4>	numEachAttrib;
	//std::vector<glm::mat4>	MM;				///< Model matrices for each rendering of this object
	//std::vector<glm::mat3>  normalMatrix;	///< Normals are passed to fragment shader in world coordinates, so they have to be multiplied by the model matrix (MM) first (this MM should not include the translation part, so we just take the upper-left 3x3 part). However, non-uniform scaling can distort normals, so we have to create a specific MM especially tailored for normal vectors.

	void dirtyResize(size_t newCount);		///< Modifies ubo size only (useful for allowing input beyond the ubo's end in case you plan to increment size later).
	size_t dirtyCount;						///< Number of dynamic UBOs, including those generated with dirtyResize()

	std::vector<char> ubo;			///< Store the UBO that will be passed to shader	<<< is alignas(16) necessary?

private:
	const std::array<size_t, 4>	attribsSize = { sizeof(glm::mat4), sizeof(glm::mat4), sizeof(glm::mat4), sizeof(glm::mat3) + 12 };	// 64, 64, 64, 36+12 (including padding for getting alignment with 16 bits)
};

#endif