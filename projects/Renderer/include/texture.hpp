#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "environment.hpp"


class Texture
{
	const char* path;
	VulkanEnvironment* e;				///< Pointer, instead of a reference, because it is assign in loadAndCreateTexture().

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

#endif