#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <list>

#include "environment.hpp"


class Texture
{
	const char* path;									///< Path to the texture file.
	VulkanEnvironment* e;								///< Pointer, instead of a reference, because it is not defined at object creation but when calling loadAndCreateTexture().

	void createTextureImage();							///< Load an image and upload it into a Vulkan object. Process: Load a texture > Copy it to a buffer > Copy it to an image > Cleanup the buffer.
	void createTextureImageView();						///< Create an image view for the texture (images are accessed through image views rather than directly).
	void createTextureSampler();						///< Create a sampler for the textures (it applies filtering and transformations).

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);							///< Used in createTextureImage() for copying the staging buffer (VkBuffer) to the texture image (VkImage). 
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);	///< Generate mipmaps

public:
	Texture(const char* path);							///< Construction using the path to a texture file.
	Texture(const Texture& obj);						///< Copy constructor.
	~Texture();

	void loadAndCreateTexture(VulkanEnvironment& e);	///< Load image and create the VkImage, VkImageView and VkSampler.

	bool fullyConstructed;

	uint32_t					 mipLevels;				///< Number of levels (mipmaps)
	VkImage						 textureImage;			///< Opaque handle to an image object.
	VkDeviceMemory				 textureImageMemory;	///< Opaque handle to a device memory object.
	VkImageView					 textureImageView;		///< Image view for the texture image (images are accessed through image views rather than directly).
	VkSampler					 textureSampler;		///< Opaque handle to a sampler object (it applies filtering and transformations to a texture). It is a distinct object that provides an interface to extract colors from a texture. It can be applied to any image you want (1D, 2D or 3D).
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

#endif