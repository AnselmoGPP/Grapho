#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <array>

//#include <vulkan/vulkan.h>		// From LunarG SDK. Used for off-screen rendering
#define GLFW_INCLUDE_VULKAN			// Makes GLFW load the Vulkan header with it
#include "GLFW/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// GLM uses OpenGL depth range [-1.0, 1.0]. This macro forces GLM to use Vulkan range [0.0, 1.0].
#define GLM_ENABLE_EXPERIMENTAL				// Required for using std::hash functions for the GLM types (since gtx folder contains experimental extensions)
#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>		// Generate transformations matrices with glm::rotate (model), glm::lookAt (view), glm::perspective (projection).
#include <glm/gtx/hash.hpp>

/*
	Content:
		- VertexType
		- VertexSet
			- VertexPCT
			- VertexPC
			- VertexPT
			- VertexPTN
*/

/// Used for configuring VertexSet. Defines the size and type of attributes the vertex is made of (Position, Color, Texture coordinates, Normal, other...).
class VertexType
{
public:
	VertexType(std::initializer_list<size_t> attribsSizes, std::initializer_list<VkFormat> attribsFormats);	//!< Constructor. Set the size (bytes) and type of each vertex attribute (Position, Color, Texture coords, Normal, other...).
	VertexType();
	~VertexType();
	VertexType& operator=(const VertexType& obj);				//!< Copy assignment operator overloading. Required for copying a VertexSet object.

	VkVertexInputBindingDescription getBindingDescription();					//!< Used for passing the binding number and the vertex stride (usually, vertexSize) to the graphics pipeline.
	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();	//!< Used for passing the format, location and offset of each vertex attribute to the graphics pipeline.

	//static const std::array<size_t, 4> attribsSize;	//!< Size of each attribute type
	//std::array<size_t, 4> numEachAttrib;			//!< Amount of each type of attribute
	std::vector<VkFormat> attribsFormats;			//!< Format (VkFormat) of each vertex attribute. E.g.: VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT...
	std::vector<size_t> attribsSizes;				//!< Size of each attribute type. E.g.: 3 * sizeof(float)...
	size_t vertexSize;								//!< Size (bytes) of a vertex object
};


/// VertexSet serves as a container for any object type, similarly to a std::vector, but storing such objects directly in bytes (char array). This allows ModelData objects store different Vertex types in a clean way (otherwise, templates and inheritance would be required, but code would be less clean).
class VertexSet
{
public:
	VertexSet(VertexType vertexType);
	//VertexSet(VertexType vertexType, size_t numOfVertex, const void* buffer);
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
	void reset(VertexType vertexType, size_t numOfVertex, const void* buffer);	/// Similar to a copy constructor, but just using its parameters instead of an already existing object.
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

#endif