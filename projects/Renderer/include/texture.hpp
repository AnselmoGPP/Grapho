#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <list>

#include <glm/glm.hpp>

#include "environment.hpp"


class Texture
{
	const char* path;									///< Path to the texture file.
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
	Texture(const char* path, VkFormat imageFormat, VkSamplerAddressMode addressMode);										//!< Construction from a texture file
	Texture(unsigned char* pixels, int texWidth, int texHeight, VkFormat imageFormat, VkSamplerAddressMode addressMode);	//!< Construction from in-code data
	//Texture(const Texture& obj);			//!< Copy constructor.
	~Texture();

	void loadAndCreateTexture(VulkanEnvironment* e);	///< Load image and create the VkImage, VkImageView and VkSampler.

	bool			fullyConstructed;		//!< Flags if this object has been fully constructed (i.e. has a texture loaded into Vulkan).
	uint32_t		mipLevels;				///< Number of levels (mipmaps)
	VkImage			textureImage;			///< Opaque handle to an image object.
	VkDeviceMemory	textureImageMemory;		///< Opaque handle to a device memory object.
	VkImageView		textureImageView;		///< Image view for the texture image (images are accessed through image views rather than directly).
	VkSampler		textureSampler;			///< Opaque handle to a sampler object (it applies filtering and transformations to a texture). It is a distinct object that provides an interface to extract colors from a texture. It can be applied to any image you want (1D, 2D or 3D).
};

typedef std::list<Texture>::iterator texIterator;

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
	std::vector<texIterator> texMaps;
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