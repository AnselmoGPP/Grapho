
#include <iostream>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "texture.hpp"
#include "commons.hpp"


Texture::Texture(const char* path, VkFormat imageFormat, VkSamplerAddressMode addressMode)
	: path(nullptr), e(nullptr), imageFormat(imageFormat), addressMode(addressMode), texWidth(0), texHeight(0), texChannels(0), pixels(nullptr), fullyConstructed(false), mipLevels(0)
{
	copyCString(this->path, path);
}

Texture::Texture(unsigned char* pixels, int texWidth, int texHeight, VkFormat imageFormat, VkSamplerAddressMode addressMode)
	: path(nullptr), e(nullptr), imageFormat(imageFormat), addressMode(addressMode), texWidth(texWidth), texHeight(texHeight), texChannels(0), pixels(nullptr), fullyConstructed(false), mipLevels(0)
{
	this->pixels = new unsigned char[4 * texHeight * texWidth];
	memcpy(this->pixels, pixels, 4 * texHeight * texWidth);
}
/*
Texture::Texture(const Texture& obj)
{
	if (this == &obj) return;
	this->path = nullptr;
	this->e = nullptr;

	if (!e)
		copyCString(this->path, obj.path);
	else
	{
		copyCString(this->path, obj.path);
		this->e = obj.e;
		this->fullyConstructed = obj.fullyConstructed;
		this->mipLevels = obj.mipLevels;
		this->textureImage = obj.textureImage;
		this->textureImageMemory = obj.textureImageMemory;
		this->textureImageView = obj.textureImageView;
		this->textureSampler = obj.textureSampler;
	}
}
*/
Texture::~Texture() 
{ 
	if(path) delete[] path;

	if (e)
	{
		vkDestroySampler(e->device, textureSampler, nullptr);
		vkDestroyImage(e->device, textureImage, nullptr);
		vkDestroyImageView(e->device, textureImageView, nullptr);
		vkFreeMemory(e->device, textureImageMemory, nullptr);
	}
}

void Texture::loadAndCreateTexture(VulkanEnvironment& e)
{
	if(path)
		std::cout << __func__ << "(): " << path << std::endl;
	else
		std::cout << __func__ << "(): " << "In-code generated texture" << std::endl;

	this->e = &e;
	
	createTextureImage();
	createTextureImageView();
	createTextureSampler();

	fullyConstructed = true;
}

// (15)
void Texture::createTextureImage()
{
	// Load an image (usually, the most expensive process)
	if (path)	// data from file
	{
		pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);		// Returns a pointer to an array of pixel values. STBI_rgb_alpha forces the image to be loaded with an alpha channel, even if it doesn't have one.
		if (!pixels) throw std::runtime_error("Failed to load texture image!");
	}
	else { }	// data from code

	VkDeviceSize imageSize = texWidth * texHeight * 4;												// 4 bytes per rgba pixel
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;	// Calculate the number levels (mipmaps)
	std::cout << __func__ << std::endl;
	// Create a staging buffer (temporary buffer in host visible memory so that we can use vkMapMemory and copy the pixels to it)
	VkBuffer	   stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(
		*e,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// Copy directly the pixel values from the image we loaded to the staging-buffer.
	void* data;
	vkMapMemory(e->device, stagingBufferMemory, 0, imageSize, 0, &data);	// vkMapMemory retrieves a host virtual address pointer (data) to a region of a mappable memory object (stagingBufferMemory). We have to provide the logical device that owns the memory (e.device).
	memcpy(data, pixels, static_cast<size_t>(imageSize));					// Copies a number of bytes (imageSize) from a source (pixels) to a destination (data).
	vkUnmapMemory(e->device, stagingBufferMemory);							// Unmap a previously mapped memory object (stagingBufferMemory).
	
	stbi_image_free(pixels);	// Clean up the original pixel array

	// Create the texture image
	e->createImage(
		texWidth, texHeight,
		mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		imageFormat,			// VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R64_SFLOAT
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureImageMemory);
	
	// Copy the staging buffer to the texture image
	e->transitionImageLayout(textureImage, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);					// Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));											// Execute the buffer to image copy operation
	// Transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
	// transitionImageLayout(textureImage, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);	// To be able to start sampling from the texture image in the shader, we need one last transition to prepare it for shader access
	generateMipmaps(textureImage, imageFormat, texWidth, texHeight, mipLevels);

	// Cleanup the staging buffer and its memory
	vkDestroyBuffer(e->device, stagingBuffer, nullptr);
	vkFreeMemory(e->device, stagingBufferMemory, nullptr);
	std::cout << __func__ << std::endl;
}

void Texture::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = e->beginSingleTimeCommands();

	// Specify which part of the buffer is going to be copied to which part of the image
	VkBufferImageCopy region{};
	region.bufferOffset = 0;							// Byte offset in the buffer at which the pixel values start
	region.bufferRowLength = 0;							// How the pixels are laid out in memory. 0 indicates that the pixels are thightly packed. Otherwise, you could have some padding bytes between rows of the image, for example. 
	region.bufferImageHeight = 0;							// How the pixels are laid out in memory. 0 indicates that the pixels are thightly packed. Otherwise, you could have some padding bytes between rows of the image, for example.
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// imageSubresource indicate to which part of the image we want to copy the pixels
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };					// Indicate to which part of the image we want to copy the pixels
	region.imageExtent = { width, height, 1 };			// Indicate to which part of the image we want to copy the pixels

	// Enqueue buffer to image copy operations
	{
		const std::lock_guard<std::mutex> lock(e->mutCommandPool);

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,			// Layout the image is currently using
			1,
			&region);
	}

	e->endSingleTimeCommands(commandBuffer);
}

void Texture::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	// Check if the image format supports linear blitting. We are using vkCmdBlitImage, but it's not guaranteed to be supported on all platforms bacause it requires our texture image format to support linear filtering, so we check it with vkGetPhysicalDeviceFormatProperties.
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(e->physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		throw std::runtime_error("Texture image format does not support linear blitting!");
		// Two alternatives:
		//		- Implement a function that searches common texture image formats for one that does support linear blitting.
		//		- Implement the mipmap generation in software with a library like stb_image_resize. Each mip level can then be loaded into the image in the same way that you loaded the original image.
		// It's uncommon to generate the mipmap levels at runtime anyway. Usually they are pregenerated and stored in the texture file alongside the base level to improve loading speed. <<<<<
	}

	VkCommandBuffer commandBuffer = e->beginSingleTimeCommands();

	// Specify the barriers
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++)	// This loop records each of the VkCmdBlitImage commands. The source mip level is i - 1 and the destination mip level is i.
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;	// We transition level i - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL. This transition will wait for level i - 1 to be filled, either from the previous blit command, or from vkCmdCopyBufferToImage. The current blit command will wait on this transition.
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		const std::lock_guard<std::mutex> lock(e->mutCommandPool);

		// Record a barrier (we transition level i - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL. This transition will wait for level i - 1 to be filled, either from the previous blit command, or from vkCmdCopyBufferToImage. The current blit command will wait on this transition).
		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		// Specify the regions that will be used in the blit operation
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };						// srcOffsets determine the 3D regions ...
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };		// ... that data will be blitted from.
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };																	// dstOffsets determine the 3D region ...
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,  mipHeight > 1 ? mipHeight / 2 : 1,  1 };	// ... that data will be blitted to.
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		// Record a blit command. Beware if you are using a dedicated transfer queue: vkCmdBlitImage must be submitted to a queue with graphics capability.
		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		// The textureImage is used for both the srcImage and dstImage parameter ...
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		// ...  because we're blitting between different levels of the same image.
			1, &blit,
			VK_FILTER_LINEAR);									// Enable interpolation

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		// Record a barrier (This barrier transitions mip level i - 1 to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. This transition waits on the current blit command to finish. All sampling operations will wait on this transition to finish).
		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	// Record a barrier (This barrier transitions the last mip level from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. This wasn't handled by the loop, since the last mip level is never blitted from).
	{
		const std::lock_guard<std::mutex> lock(e->mutCommandPool);

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	e->endSingleTimeCommands(commandBuffer);
}

// (16)
void Texture::createTextureImageView()
{
	textureImageView = e->createImageView(textureImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

// (17)
void Texture::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;					// How to interpolate texels that are magnified (oversampling) or ...
	samplerInfo.minFilter = VK_FILTER_LINEAR;					// ... minified (undersampling). Choices: VK_FILTER_NEAREST, VK_FILTER_LINEAR
	samplerInfo.addressModeU = addressMode;						// Addressing mode per axis (what happens when going beyond the image dimensions). In texture space coordinates, XYZ are UVW. Available values: VK_SAMPLER_ADDRESS_MODE_ ... REPEAT (repeat the texture), MIRRORED_REPEAT (like repeat, but inverts coordinates to mirror the image), CLAMP_TO_EDGE (take the color of the closest edge), MIRROR_CLAMP_TO_EDGE (like clamp to edge, but taking the opposite edge), CLAMP_TO_BORDER (return solid color).
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;

	if (e->supportsAF)		// If anisotropic filtering is available (see isDeviceSuitable) <<<<<
	{
		samplerInfo.anisotropyEnable = VK_TRUE;							// Specify if anisotropic filtering should be used
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(e->physicalDevice, &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;		// another option:  samplerInfo.maxAnisotropy = 1.0f;
	}
	else
	{
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
	}

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Color returned (black, white or transparent, in format int or float) when sampling beyond the image with VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER. You cannot specify an arbitrary color.
	samplerInfo.unnormalizedCoordinates = VK_FALSE;							// Coordinate system to address texels in an image. False: [0, 1). True: [0, texWidth) & [0, texHeight). 
	samplerInfo.compareEnable = VK_FALSE;							// If a comparison function is enabled, then texels will first be compared to a value, and the result of that comparison is used in filtering operations. This is mainly used for percentage-closer filtering on shadow maps (https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing). 
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;	// VK_SAMPLER_MIPMAP_MODE_ ... NEAREST (lod selects the mip level to sample from), LINEAR (lod selects 2 mip levels to be sampled, and the results are linearly blended)
	samplerInfo.minLod = 0.0f;								// minLod=0 & maxLod=mipLevels allow the full range of mip levels to be used
	samplerInfo.maxLod = static_cast<float>(mipLevels);	// lod: Level Of Detail
	samplerInfo.mipLodBias = 0.0f;								// Used for changing the lod value. It forces to use lower "lod" and "level" than it would normally use

	if (vkCreateSampler(e->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture sampler!");
	/*
	* VkImage holds the mipmap data. VkSampler controls how that data is read while rendering.
	* The sampler selects a mip level according to this pseudocode:
	*
	*	lod = getLodLevelFromScreenSize();						// Smaller when the object is close (may be negative)
	*	lod = clamp(lod + mipLodBias, minLod, maxLod);
	*
	*	level = clamp(floor(lod), 0, texture.miplevels - 1);	// Clamped to the number of miplevels in the texture
	*
	*	if(mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST)		// Sample operation
	*		color = sampler(level);
	*	else
	*		color = blend(sample(level), sample(level + 1));
	*
	*	if(lod <= 0)											// Filter
	*		color = readTexture(uv, magFilter);
	*	else
	*		color = readTexture(uv, minFilter);
	*/
}

// getOpticalDepthTable -----------------------------------------------------------

/*
	getOpticalDepthTable
		operator ()
			raySphere()
			opticalDepth()
				densityAtPoint()
*/

OpticalDepthTable::OpticalDepthTable(unsigned numOptDepthPoints, unsigned planetRadius, unsigned atmosphereRadius, float heightStep, float angleStep, float densityFallOff)
		: planetRadius(planetRadius), atmosphereRadius(atmosphereRadius), numOptDepthPoints(numOptDepthPoints), heightStep(heightStep), angleStep(angleStep), densityFallOff(densityFallOff)
{
	// Compute useful variables	
	heightSteps = std::ceil(1 + (atmosphereRadius - planetRadius) / heightStep);	// <<<
	angleSteps = std::ceil(1 + 3.141592653589793238462 / angleStep);
	bytes = 4 * heightSteps * angleSteps;	// sizeof(float) = 4
	
	// Get table
	table.resize(bytes);

	float rayLength, angle;
	glm::vec3 point, rayDir;
	float* optDepth = (float*)table.data();

	for (size_t i = 0; i < heightSteps; i++)
	{
		point = { 0, planetRadius + i * heightStep, 0 };	// rayOrigin

		for (size_t j = 0; j < angleSteps; j++)
		{
			angle = j * angleStep;
			rayDir = glm::vec3(sin(angle), cos(angle), 0);
			rayLength = raySphere(point, rayDir).y;

			optDepth[i * angleSteps + j] = opticalDepth(point, rayDir, rayLength);
			
			//if (point.y > 2399 && point.y < 2401 && angle > 1.84 && angle < 1.86)
			//	std::cout << "(" << i << ", " << j << ") / " << point.y << " / " << optDepth[i * angleSteps + j] << " / " << rayLength << " / " << angle << " / (" << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << ")" << std::endl;
		}
	}

	// Compute
	float angleRange = 3.141592653589793238462;
	point = glm::vec3(2400, 0, 0);
	angle = angleRange / 1.7;
	rayDir = glm::vec3(cos(angle), 0, sin(angle));
	rayLength = raySphere(point, rayDir).y;
	std::cout << ">>> " << opticalDepth(point, rayDir, rayLength) << std::endl;
	std::cout << angle << std::endl;

	// Look up
	float heightRatio = (2400.f - planetRadius) / (atmosphereRadius - planetRadius);
	float angleRatio = angle / angleRange;
	unsigned i = (heightSteps - 1) * heightRatio;
	unsigned j = (angleSteps - 1) * angleRatio;
	//std::cout << planetRadius << ", " << atmosphereRadius << std::endl;
	//std::cout << ">>> (" << i << ", " << j << ") / " << optDepth[i * angleSteps + j] << std::endl;
}

float OpticalDepthTable::opticalDepth(glm::vec3 rayOrigin, glm::vec3 rayDir, float rayLength) const
{
	glm::vec3 point = rayOrigin;
	float stepSize = rayLength / (numOptDepthPoints - 1);
	float opticalDepth = 0;

	for (int i = 0; i < numOptDepthPoints; i++)
	{
		opticalDepth += densityAtPoint(point) * stepSize;
		point += rayDir * stepSize;
	}

	return opticalDepth;
}

float OpticalDepthTable::densityAtPoint(glm::vec3 point) const
{
	float heightAboveSurface = glm::length(point - planetCenter) - planetRadius;
	float height01 = heightAboveSurface / (atmosphereRadius - planetRadius);

	//return exp(-height01 * densityFallOff);					// There is always some density
	return exp(-height01 * densityFallOff) * (1 - height01);	// Density ends at some distance
}

// Returns distance to sphere surface. If it's not, return maximum floating point.
// Returns vector(distToSphere, distThroughSphere). 
//		If rayOrigin is inside sphere, distToSphere = 0. 
//		If ray misses sphere, distToSphere = maxValue; distThroughSphere = 0.
glm::vec2 OpticalDepthTable::raySphere(glm::vec3 rayOrigin, glm::vec3 rayDir) const
{
	//std::cout << ">>> " << rayOrigin.x << ", " << rayOrigin.y << " / " << rayDir.x << ", " << rayDir.y << std::endl;

	// Number of intersections
	glm::vec3 offset = rayOrigin - planetCenter;
	float a = 1;						// Set to dot(rayDir, rayDir) if rayDir might not be normalized
	float b = 2 * dot(offset, rayDir);
	float c = glm::dot(offset, offset) - atmosphereRadius * atmosphereRadius;
	float d = b * b - 4 * a * c;		// Discriminant of quadratic formula (sqrt has 2 solutions/intersections when positive)

	// Two intersections (d > 0)
	if (d > 0)
	{
		float s = sqrt(d);
		float distToSphereNear = std::max(0.f, (-b - s) / (2 * a));
		float distToSphereFar = (-b + s) / (2 * a);

		if (distToSphereFar >= 0)		// Ignore intersections that occur behind the ray
			return glm::vec2(distToSphereNear, distToSphereFar - distToSphereNear);
	}

	// No intersection (d < 0) or one (d = 0)
	return glm::vec2(FLT_MAX, 0);			// https://stackoverflow.com/questions/16069959/glsl-how-to-ensure-largest-possible-float-value-without-overflow

	/*
		/ Line:     y = mx + b
		\ Circle:   r^2 = x^2 + y^2;	y = sqrt(r^2 - x^2)
					r^2 = (x - h)^2 + (y - k)^2;	r^2 = X^2 + x^2 + 2Xx + Y^2 + y^2 + 2Yy

		mx + b = sqrt(r^2 - x^2)
		mmx^2 + b^2 + 2mbx = r^2 - x^2
		mmx^2 + b^2 + 2mbx - r^2 + x^2  = 0
		(mm + 1)x^2 + 2mbx + (b^2 - r^2) = 0 
	*/

	//float m = rayDir.y / rayDir.x;	// line's slope
	//float B = rayOrigin.y;			// line's Y-intercept 
	//float a = m * m + 1;
	//float b = 2 * m * B;
	//float c = B * B - atmosphereRadius * atmosphereRadius;
	//float d = b * b - 4 * a * c;
}

DensityVector::DensityVector(float planetRadius, float atmosphereRadius, float stepSize, float densityFallOff)
{
	heightSteps = std::ceil((atmosphereRadius - planetRadius) / stepSize);
	bytes = 4 * heightSteps;
	table.resize(bytes);

	glm::vec2 point = { 0.f, planetRadius };
	glm::vec2 planetCenter = { 0.f, 0.f };
	float heightAboveSurface;
	float height01;
	float* density = (float*)table.data();

	for (size_t i = 0; i < heightSteps; i++)
	{
		heightAboveSurface = glm::length(point - planetCenter) - planetRadius;
		height01 = heightAboveSurface / (atmosphereRadius - planetRadius);

		//density[i] = std::exp(-height01 * densityFallOff);					// There is always some density
		density[i] = std::exp(-height01 * densityFallOff) * (1 - height01);	// Density ends at some distance

		point.y += stepSize;
	}
}
