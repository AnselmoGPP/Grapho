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
		VertexSet
			VertexPCT
			VertexPC
			VertexPT

		UBOdynamic (UBOset)
			UBO_MVP
*/

//enum vertexType{ PCT, PC, PT };
enum vertexElements{ position, color, texture };


class VertexType
{
public:
	VertexType(size_t vertexSize, size_t numP, size_t numC, size_t numT);
	~VertexType();
	VertexType& operator=(const VertexType& obj);

	VkVertexInputBindingDescription getBindingDescription();
	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
	
	//static const size_t maxNumAttribs = 3;				// Number of types of attributes (Positions, Color buffers, Texture coordinates buffers)
	const std::array<size_t, 3> attribsSize = { sizeof(glm::vec3), sizeof(glm::vec3), sizeof(glm::vec2) };

	//char* vertex;							// Vertex data
	size_t vertexSize;						// LOOK necessary? Size in bytes of a vertex object (including padding)
	std::array<size_t, 3> numEachAttrib;	// Amount of each type of attribute
};

/// VertexSet serves as a container for any object type, similarly to a std::vector, but storing such objects directly in bytes (char array). This allows ModelData objects store different Vertex types in a clean way (otherwise, templates and inheritance would be required, but code would be less clean).
class VertexSet
{
public:
	VertexSet(VertexType vertexType);
	VertexSet(VertexType vertexType, size_t numOfVertex, const void* buffer);
	~VertexSet();
	VertexSet& operator=(const VertexSet& obj);

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


/// Model-View-Projection matrix as a UBO (Uniform buffer object) (https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/)
struct UBO_MVP {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

/// Structure used for storing many UBOs in the same structure in order to allow us to render the same model many times.
struct UBOdynamic
{
	UBOdynamic(size_t subUBOcount, VkDeviceSize minSizePerSubUBO);
	~UBOdynamic();

	void setModel(size_t position, const glm::mat4& matrix);
	void setView(size_t position, const glm::mat4& matrix);
	void setProj(size_t position, const glm::mat4& matrix);

	alignas(16) size_t			UBOcount;
	alignas(16) VkDeviceSize	sizePerUBO;
	alignas(16) size_t			totalBytes;

	alignas(16) char* data;			// <<< is alignas(16) necessary?
};

#endif