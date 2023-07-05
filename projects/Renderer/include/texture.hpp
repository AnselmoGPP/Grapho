#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <list>

#include <glm/glm.hpp>

#include "shaderc/shaderc.hpp"		// Compile GLSL code to SPIR-V

#include "assimp/Importer.hpp"		// Import textures
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "environment.hpp"
#include "vertex.hpp"

//#define DEBUG_RESOURCES

extern VertexType vt_32;					//!< (Vert, UV)
extern VertexType vt_33;					//!< (Vert, Color)
extern VertexType vt_332;					//!< (Vert, Normal, UV)
extern VertexType vt_333;					//!< (Vert, Normal, vertexFixes)

class Shader;
class Texture;
class TextureInfo;

typedef std::list<Shader >::iterator shaderIter;
typedef std::list<Texture>::iterator texIter;

extern std::vector<TextureInfo> noTextures;		//!< Vector with 0 TextureInfo objects
extern std::vector<uint16_t   > noIndices;			//!< Vector with 0 indices

struct ResourcesInfo;


// VERTICES --------------------------------------------------------

class VerticesLoader
{
public:
	VerticesLoader(const VertexType& vertexType);
	virtual ~VerticesLoader() { };

	virtual void loadVertices(VertexSet& vertices, std::vector<uint16_t>& indices, ResourcesInfo* resourcesInfo) = 0;
	virtual VerticesLoader* clone() = 0;

	const VertexType vertexType;
};

class VerticesFromBuffer : public VerticesLoader
{
	//size_t vertexCount;

public:
	VerticesFromBuffer(const VertexType& vertexType, const void* verticesData, size_t vertexCount, const std::vector<uint16_t>& indices, VertexSet& destVertices, std::vector<uint16_t>& destIndices);

	void loadVertices(VertexSet& vertices, std::vector<uint16_t>& indices, ResourcesInfo* resourcesInfo) override;
	VerticesLoader* clone() override;
};

class VerticesFromFile : public VerticesLoader
{
	std::string path;

	VertexSet* vertices;
	std::vector<uint16_t>* indices;
	ResourcesInfo* resources;

	void processNode(aiNode* node, const aiScene* scene);	//!< Recursive function. It goes through each node getting all the meshes in each one.
	void processMesh(aiMesh* mesh, const aiScene* scene);

public:
	VerticesFromFile(const VertexType& vertexType, std::string& filePath);

	void loadVertices(VertexSet& vertices, std::vector<uint16_t>& indices, ResourcesInfo* resourcesInfo) override;
	VerticesLoader* clone() override;
};


class VerticesInfo
{
	//std::string path;

	//std::vector<char> vertexData;
	//std::vector<uint16_t> indices;

	//void processNode(aiNode* node, const aiScene* scene);	//!< Recursive function. It goes through each node getting all the meshes in each one.
	//void processMesh(aiMesh* mesh, const aiScene* scene);

public:
	VerticesInfo(const VertexType& vertexType, std::string& filePath);	//!< From file <<< Can vertexType be taken from file?
	VerticesInfo(const VertexType& vertexType, const void* verticesData, size_t vertexCount, std::vector<uint16_t>& indices);	//!< from buffers
	VerticesInfo(const VerticesInfo& obj);	//!< Copy constructor (necessary because loader can be freed in destructor)
	~VerticesInfo();

	VertexSet vertices;
	std::vector<uint16_t> indices;

	VerticesLoader* loader;

	void loadVertices(ResourcesInfo* resourcesInfo);

	////const std::string id;
	//const VertexType vertexType;
	//size_t vertexCount;
};


// VERTICES --------------------------------------------------------

class Shader
{
public:
	Shader(VulkanEnvironment& e, const std::string id, VkShaderModule shaderModule);
	~Shader();

	VulkanEnvironment& e;
	const std::string id;						//!< Used for checking whether the shader to load is already loaded.
	unsigned counter;							//!< Number of ModelData objects using this shader.

	const VkShaderModule shaderModule;
};

typedef std::list<Shader>::iterator shaderIter;

class ShaderInfo
{
	std::string filePath;

	std::vector<char> glslData;	// <<< pointer instead?

public:
	ShaderInfo(std::string& filePath);
	ShaderInfo(const std::string& id, std::string& text);
	ShaderInfo& operator=(const ShaderInfo& obj);

	std::list<Shader>::iterator loadShader(VulkanEnvironment& e, std::list<Shader>& loadedShaders);

	std::string id;
};

/**
	Includer interface for being able to "#include" headers data on shaders
	Renderer::newShader():
		- readFile(shader)
		- shaderc::CompileOptions < ShaderIncluder
		- shaderc::Compiler::PreprocessGlsl()
			- Preprocessor directive exists?
				- ShaderIncluder::GetInclude()
				- ShaderIncluder::ReleaseInclude()
				- ShaderIncluder::~ShaderIncluder()
		- shaderc::Compiler::CompileGlslToSpv
*/
class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
	~ShaderIncluder() { };

	// Handles shaderc_include_resolver_fn callbacks.
	shaderc_include_result* GetInclude(const char* sourceName, shaderc_include_type type, const char* destName, size_t includeDepth) override;

	// Handles shaderc_include_result_release_fn callbacks.
	void ReleaseInclude(shaderc_include_result* data) override;
};


// TEXTURE --------------------------------------------------------

class Texture
{
public:
	Texture(VulkanEnvironment& e, const std::string& id, VkImage textureImage, VkDeviceMemory textureImageMemory, VkImageView textureImageView, VkSampler textureSampler);
	~Texture();

	VulkanEnvironment& e;
	const std::string id;						//!< Used for checking whether the texture to load is already loaded.
	unsigned counter;							//!< Number of ModelData objects using this texture.

	VkImage				textureImage;			//!< Opaque handle to an image object.
	VkDeviceMemory		textureImageMemory;		//!< Opaque handle to a device memory object.
	VkImageView			textureImageView;		//!< Image view for the texture image (images are accessed through image views rather than directly).
	VkSampler			textureSampler;			//!< Opaque handle to a sampler object (it applies filtering and transformations to a texture). It is a distinct object that provides an interface to extract colors from a texture. It can be applied to any image you want (1D, 2D or 3D).
};

class TextureInfo
{
	std::string filePath;

	unsigned char* pixels;
	int texWidth, texHeight;

	std::pair<VkImage, VkDeviceMemory> createTextureImage();
	VkImageView                        createTextureImageView(VkImage textureImage);
	VkSampler                          createTextureSampler();

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	VulkanEnvironment* e;
	uint32_t mipLevels;				//!< Number of levels (mipmaps)

public:
	TextureInfo(std::string filePath, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
	TextureInfo(unsigned char* pixels, int texWidth, int texHeight, std::string id, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
	TextureInfo& operator=(const TextureInfo& obj);
	~TextureInfo();

	std::list<Texture>::iterator loadTexture(VulkanEnvironment& e, std::list<Texture>& loadedTextures);

	std::string id;
	VkFormat imageFormat;
	VkSamplerAddressMode addressMode;
};


// RESOURCES --------------------------------------------------------

struct ResourcesInfo
{
	ResourcesInfo(VerticesInfo& verticesInfo, std::vector<ShaderInfo>& shadersInfo, std::vector<TextureInfo>& texturesInfo);

	VerticesInfo vertices;
	std::vector<ShaderInfo> shaders;
	std::vector<TextureInfo> textures;


};


// OTHERS --------------------------------------------------------

class Texture0
{
	std::string path;									///< Path to the texture file.
	VulkanEnvironment* e;								///< Pointer, instead of a reference, because it is not defined at object creation but when calling loadAndCreateTexture().
	VkFormat imageFormat;								//!< VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R32_SFLOAT, ...
	VkSamplerAddressMode addressMode;					//!< VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, ...
	//VkFilter magMinFilter;							//!< VK_FILTER_LINEAR, VK_FILTER_NEAREST, ...

	void createTextureImage();							///< Load an image and upload it into a Vulkan object. Process: Load a texture > Copy it to a buffer > Copy it to an image > Cleanup the buffer.
	void createTextureImageView();						///< Create an image view for the texture (images are accessed through image views rather than directly).
	void createTextureSampler();						///< Create a sampler for the textures (it applies filtering and transformations).

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);							///< Used in createTextureImage() for copying the staging buffer (VkBuffer) to the texture image (VkImage). 
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);	///< Generate mipmaps

	// Temporals for loading data from code
	int texWidth, texHeight, texChannels;
	unsigned char* pixels;					//!< stbi_uc* pixels

public:
	Texture0(std::string path, VkFormat imageFormat, VkSamplerAddressMode addressMode);										//!< Construction from a texture file
	Texture0(unsigned char* pixels, int texWidth, int texHeight, VkFormat imageFormat, VkSamplerAddressMode addressMode);	//!< Construction from in-code data
	//Texture(const Texture& obj);			//!< Copy constructor.
	~Texture0();

	void loadAndCreateTexture(VulkanEnvironment* e);	//!< Load image and create the VkImage, VkImageView and VkSampler. Used in Renderer::loadingThread()

	bool			fullyConstructed;		//!< Flags if this object has been fully constructed (i.e. has a texture loaded into Vulkan).
	uint32_t		mipLevels;				//!< Number of levels (mipmaps)
	VkImage			textureImage;			//!< Opaque handle to an image object.
	VkDeviceMemory	textureImageMemory;		//!< Opaque handle to a device memory object.
	VkImageView		textureImageView;		//!< Image view for the texture image (images are accessed through image views rather than directly).
	VkSampler		textureSampler;			//!< Opaque handle to a sampler object (it applies filtering and transformations to a texture). It is a distinct object that provides an interface to extract colors from a texture. It can be applied to any image you want (1D, 2D or 3D).
};

/**
	@struct PBRmaterial
	@brief Set of texture maps used for PBR (Physically Based Rendering). Passed to shader as textures.

	Commonly used maps:
	<ul>
		<li>Diffuse/Albedo </li>
		<li>Specular/Roughness/Smoothness(inverted) </li>
		<li>Shininess (Metallic?) </li>
		<li>Normals </li>
		<li>Ambient occlusion </li>
		<li>Height maps (for geometry shader?) </li>
		<li>Emissive </li>
		<li>Subsurface scattering </li>
	</ul>
*/
struct PBRmaterial
{
	std::vector<texIter> texMaps;
};


/// Precompute all optical depth values through the atmosphere. Useful for creating a lookup table for atmosphere rendering.
class OpticalDepthTable
{
	glm::vec3 planetCenter{ 0.f, 0.f, 0.f };
	unsigned planetRadius;
	unsigned atmosphereRadius;
	unsigned numOptDepthPoints;
	float heightStep;
	float angleStep;
	float densityFallOff;

	float opticalDepth(glm::vec3 rayOrigin, glm::vec3 rayDir, float rayLength) const;
	float densityAtPoint(glm::vec3 point) const;
	glm::vec2 raySphere(glm::vec3 rayOrigin, glm::vec3 rayDir) const;

public:
	OpticalDepthTable(unsigned numOptDepthPoints, unsigned planetRadius, unsigned atmosphereRadius, float heightStep, float angleStep, float densityFallOff);

	std::vector<unsigned char> table;
	size_t heightSteps;
	size_t angleSteps;
	size_t bytes;
};

/// Precompute all density values through the atmosphere. Useful for creating a lookup table for atmosphere rendering.
class DensityVector
{
public:
	DensityVector(float planetRadius, float atmosphereRadius, float stepSize, float densityFallOff);

	std::vector<unsigned char> table;
	size_t heightSteps;
	size_t bytes;
};


#endif