#ifndef MODELS_HPP
#define MODELS_HPP

#include <iostream>
#include <array>
#include <functional>						// std::function (function wrapper that stores a callable object)

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// GLM uses OpenGL depth range [-1.0, 1.0]. This macro forces GLM to use Vulkan range [0.0, 1.0].
#define GLM_ENABLE_EXPERIMENTAL				// Required for using std::hash functions for the GLM types (since gtx folder contains experimental extensions)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>		// Generate transformations matrices with glm::rotate (model), glm::lookAt (view), glm::perspective (projection).
#include <glm/gtx/hash.hpp>

#include "environment.hpp"


struct Vertex
{
	Vertex() = default;
	Vertex(glm::vec3 vertex, glm::vec3 vertexColor, glm::vec2 textureCoordinates);

	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription					getBindingDescription();	///< Describes at which rate to load data from memory throughout the vertices (number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance).
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();	///< Describe how to extract a vertex attribute from a chunk of vertex data originiating from a binding description. Two attributes here: position and color.
	bool operator==(const Vertex& other) const;											///< Overriding of operator ==. Required for doing comparisons in loadModel().
};

/// Model-View-Projection matrix as a UBO (Uniform buffer object) (https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/)
struct UniformBufferObject {
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
	void setView (size_t position, const glm::mat4& matrix);
	void setProj (size_t position, const glm::mat4& matrix);

	alignas(16) size_t			UBOcount;
	alignas(16) VkDeviceSize	sizePerUBO;
	alignas(16) size_t			totalBytes;

	alignas(16) char*			data;			// <<< is alignas(16) necessary?
};

/// Hash function for Vertex. Implemented by specifying a template specialization for std::hash<T> (https://en.cppreference.com/w/cpp/utility/hash). Required for doing comparisons in loadModel().
template<> struct std::hash<Vertex> {
	size_t operator()(Vertex const& vertex) const;
};

class ModelData
{
	VulkanEnvironment& e;

	const char* modelPath;
	
	const char* VSpath;
	const char* FSpath;

	bool dataFromFile;						// Tell if vertex-color-texture_index comes from file or from code

	// Main methods:

	void createDescriptorSetLayout();		///< Layout for the descriptor set (descriptor: handle or pointer into a resource (buffer, sampler, texture...))
	void createGraphicsPipeline(const char* VSpath, const char* FSpath);///< Create the graphics pipeline.

	void createTextureImage(const char* path);///< Load an image and upload it into a Vulkan object.
	void createTextureImageView();			///< Create an image view for the texture (images are accessed through image views rather than directly).
	void createTextureSampler();			///< Create a sampler for the textures (it applies filtering and transformations).
	void loadModel(const char* obj_file);	///< Populate the vertices and indices members with the vertex data from the mesh (OBJ file).
	void createVertexBuffer();				///< Vertex buffer creation.
	void createIndexBuffer();				///< Index buffer creation
	void createUniformBuffers();			///< Uniform buffer creation (type of descriptors that can be bound), one for each swap chain image.
	void createDescriptorPool();			///< Descriptor pool creation (a descriptor set for each VkBuffer resource to bind it to the uniform buffer descriptor).
	void createDescriptorSets();			///< Descriptor sets creation.

	// Helper methods:

	static std::vector<char>	readFile(/*const std::string& filename*/ const char* filename);	///< Read all of the bytes from the specified file and return them in a byte array managed by a std::vector.
	VkShaderModule				createShaderModule(const std::vector<char>& code);				///< Take a buffer with the bytecode as parameter and create a VkShaderModule from it.
	void						createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);	///< Helper function for creating a buffer (VkBuffer and VkDeviceMemory).
	void						copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void						generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void						copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void						copyCString(const char*& destination, const char* source);

public:
	ModelData(VulkanEnvironment &environment, size_t numberOfRenderings, const char* modelPath, const char* texturePath, const char* VSpath, const char* FSpath, bool partialInitialization = false);
	ModelData(VulkanEnvironment& environment, size_t numberOfRenderings, std::vector<Vertex>& vertexData, std::vector<uint32_t>& indicesData, const char* texturePath, const char* VSpath, const char* FSpath, bool partialInitialization = false);
	ModelData(const ModelData& obj);	// Copy constructor not used
	~ModelData();

	ModelData& fullConstruction();

	void recreateSwapChain();
	void cleanupSwapChain();
	void cleanup();

	VkDescriptorSetLayout		 descriptorSetLayout;	///< Opaque handle to a descriptor set layout object (combines all of the descriptor bindings).
	VkPipelineLayout			 pipelineLayout;		///< Pipeline layout. Allows to use uniform values in shaders (globals similar to dynamic state variables that can be changed at drawing at drawing time to alter the behavior of your shaders without having to recreate them).
	VkPipeline					 graphicsPipeline;		///< Opaque handle to a pipeline object.

	uint32_t					 mipLevels;				///< Number of levels (mipmaps)
	VkImage						 textureImage;			///< Opaque handle to an image object.
	VkDeviceMemory				 textureImageMemory;	///< Opaque handle to a device memory object.
	VkImageView					 textureImageView;		///< Image view for the texture image (images are accessed through image views rather than directly).
	VkSampler					 textureSampler;		///< Opaque handle to a sampler object (it applies filtering and transformations to a texture). It is a distinct object that provides an interface to extract colors from a texture. It can be applied to any image you want (1D, 2D or 3D).

	std::vector<Vertex>			 vertices;				///< Vertices of our model (vertices, color, texture coordinates)
	std::vector<uint32_t>		 indices;				///< Indices of our model (indices(
	VkBuffer					 vertexBuffer;			///< Opaque handle to a buffer object (here, vertex buffer).
	VkDeviceMemory				 vertexBufferMemory;	///< Opaque handle to a device memory object (here, memory for the vertex buffer).
	VkBuffer					 indexBuffer;			///< Opaque handle to a buffer object (here, index buffer).
	VkDeviceMemory				 indexBufferMemory;		///< Opaque handle to a device memory object (here, memory for the index buffer).

	std::vector<VkBuffer>		 uniformBuffers;		///< Opaque handle to a buffer object (here, uniform buffer). One for each swap chain image.
	std::vector<VkDeviceMemory>	 uniformBuffersMemory;	///< Opaque handle to a device memory object (here, memory for the uniform buffer). One for each swap chain image.

	VkDescriptorPool			 descriptorPool;		///< Opaque handle to a descriptor pool object.
	std::vector<VkDescriptorSet> descriptorSets;		///< List. Opaque handle to a descriptor set object. One for each swap chain image.
	
	//std::vector <std::function<glm::mat4(float)>> getModelMatrix;	///< Callbacks required in loopManager::updateUniformBuffer() for each model to render.
	//glm::mat4(*getModelMatrix) (float time);

	std::vector<uint32_t>	dynamicOffsets;	///< Stores the offsets for each ubo descriptor
	size_t					numMM;
	std::vector<glm::mat4>	MM;				///< Model matrices for each rendering of this object


	void resizeUBOset(size_t newSize, bool objectAlreadyConstructed = true);	///< Set up dynamic offsets and number of MM (model matrices)
	void setUBO(size_t pos, glm::mat4& newValue);

	const char* texturePath;
	VkDeviceSize				getUBOSize();		///< Total UBO size (example: 12)
	VkDeviceSize				getUBORange();		///< Minimum size where the useful UBO fits (example: 4)
	VkDeviceSize				getUsefulUBOSize();	///< Part of the range (useful UBO) that we will use (example: 3)
};

#endif