
#include <iostream>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shaderc/shaderc.hpp"		// Compile GLSL code to SPIR-V

#include "texture.hpp"
#include "commons.hpp"


VertexType vt_32 ({ 3 * sizeof(float), 2 * sizeof(float) }, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT });
VertexType vt_33 ({ 3 * sizeof(float), 3 * sizeof(float) }, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT });
VertexType vt_332({ 3 * sizeof(float), 3 * sizeof(float), 2 * sizeof(float) }, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT });
VertexType vt_333({ 3 * sizeof(float), 3 * sizeof(float), 3 * sizeof(float) }, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT });

std::vector<TextureInfo> noTextures;
std::vector<uint16_t   > noIndices;

// RESOURCES --------------------------------------------------------

ResourcesInfo::ResourcesInfo(VerticesInfo& verticesInfo, std::vector<ShaderInfo>& shadersInfo, std::vector<TextureInfo>& texturesInfo)
	: vertices(verticesInfo), shaders(shadersInfo), textures(texturesInfo) { }


// VERTICES --------------------------------------------------------

VerticesInfo::VerticesInfo(const VertexType& vertexType, std::string& filePath)
	: vertexType(vertexType), path(filePath) { }

VerticesInfo::VerticesInfo(const VertexType& vertexType, const void* vertexData, size_t vertexCount, std::vector<uint16_t>& indices)
	: vertexType(vertexType), indices(indices), vertexCount(vertexCount) 
{ 
	size_t bytesCount = vertexCount * vertexType.vertexSize;
	this->vertexData.resize(bytesCount);
	std::copy((char*)vertexData, (char*)vertexData + bytesCount, this->vertexData.data());
}

void VerticesInfo::loadVertices(VertexSet& vertices, std::vector<uint16_t>& indices, ResourcesInfo* resourcesInfo)
{
	#ifdef DEBUG_RESOURCES
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	destVertices = &vertices;
	destIndices = &indices;
	destResources = resourcesInfo;

	if (vertexData.size())	// from buffer
	{
		destVertices->reset(vertexType, vertexCount, vertexData.data());
		*destIndices = this->indices;
	}
	else if (path.size())	// from file
	{
		destVertices->reset(vertexType);

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}

		processNode(scene->mRootNode, scene);
	}
	else 
		std::cout << "Vertices could not be loaded" << std::endl;
}

void VerticesInfo::processNode(aiNode* node, const aiScene* scene)
{
	// Process all node's meshes
	aiMesh* mesh;
	//std::vector<aiMesh*> meshes;

	for (unsigned i = 0; i < node->mNumMeshes; i++)
	{
		mesh = scene->mMeshes[node->mMeshes[i]];
		//meshes.push_back(mesh);
		processMesh(mesh, scene);
	}

	// Repeat process in children
	for (unsigned i = 0; i < node->mNumChildren; i++)
		processNode(node->mChildren[i], scene);
}

void VerticesInfo::processMesh(aiMesh* mesh, const aiScene* scene)
{
	//<<< destVertices->reserve(destVertices->size() + mesh->mNumVertices);
	float* vertex = new float[vertexType.vertexSize / sizeof(float)];	// [3 + 3 + 2]
	unsigned i, j;

	// Get VERTEX data (positions, normals, UVs) and store it.
	for (i = 0; i < mesh->mNumVertices; i++)
	{
		vertex[0] = mesh->mVertices[i].x;
		vertex[1] = mesh->mVertices[i].y;
		vertex[2] = mesh->mVertices[i].z;

		if (mesh->mNormals)
		{
			vertex[3] = mesh->mNormals[i].x;
			vertex[4] = mesh->mNormals[i].y;
			vertex[5] = mesh->mNormals[i].z;
		}
		else { vertex[3] = 0.f; vertex[4] = 0.f; vertex[5] = 1.f; };

		if (mesh->mTextureCoords[0])
		{
			vertex[6] = mesh->mTextureCoords[0][i].x;
			vertex[7] = mesh->mTextureCoords[0][i].y;
		}
		else { vertex[6] = 0.f; vertex[7] = 0.f; };

		destVertices->push_back(vertex);
	}

	delete[] vertex;

	// Get INDICES and store them.
	aiFace face;
	for (i = 0; i < mesh->mNumFaces; i++)
	{
		face = mesh->mFaces[i];
		for (j = 0; j < face.mNumIndices; j++)
			destIndices->push_back(face.mIndices[j]);
	}
	
	// Process material
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiTextureType types[] = { aiTextureType_DIFFUSE, aiTextureType_SPECULAR };
		aiString fileName;

		for(unsigned i = 0; i < 2; i++)
			for (unsigned j = 0; j < material->GetTextureCount(types[i]); j++)
			{
				material->GetTexture(types[i], j, &fileName);		// get texture file location
				destResources->textures.push_back(TextureInfo(fileName.C_Str()));
				fileName.Clear();
			}
	}
}


// SHADERS --------------------------------------------------------

Shader::Shader(VulkanEnvironment& e, const std::string id, VkShaderModule shaderModule) 
	: e(e), id(id), counter(0), shaderModule(shaderModule) { }

Shader::~Shader() { vkDestroyShaderModule(e.c.device, shaderModule, nullptr); }

ShaderInfo::ShaderInfo(std::string& filePath)
	: id(filePath), filePath(filePath) { }

ShaderInfo::ShaderInfo(const std::string& id, std::string& text)
	: id(id) 
{ 
	glslData.resize(text.size()); 
	std::copy(text.begin(), text.end(), glslData.begin()); 
}

ShaderInfo& ShaderInfo::operator=(const ShaderInfo& obj)
{
	filePath = obj.filePath;
	glslData = obj.glslData;
	id = obj.id;

	return *this;
}

std::list<Shader>::iterator ShaderInfo::loadShader(VulkanEnvironment& e, std::list<Shader>& loadedShaders)
{
	#ifdef DEBUG_RESOURCES
		std::cout << typeid(*this).name() << "::" << __func__ << ": " << this->id << std::endl;
	#endif

	// Look for it in loadedShaders
	for(auto i = loadedShaders.begin(); i != loadedShaders.end(); i++)
		if (i->id == id) return i;

	// Load shader (if not loaded yet)
	if (filePath.size())				// from file
		readFile(id.c_str(), glslData);

	// Compile data (preprocessing > compilation):

	shaderc::CompileOptions options;
	options.SetIncluder(std::make_unique<ShaderIncluder>());
	options.SetGenerateDebugInfo();
	//if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_performance);	// This option makes shaderc::CompileGlslToSpv fail when Assimp::Importer is present in code, even if an Importer object is not created (odd) (Importer is in DataFromFile2::loadVertex).

	shaderc::Compiler compiler;

	shaderc::PreprocessedSourceCompilationResult preProcessed = compiler.PreprocessGlsl(glslData.data(), glslData.size(), shaderc_glsl_infer_from_source, id.c_str(), options);
	if (preProcessed.GetCompilationStatus() != shaderc_compilation_status_success)
		std::cerr << "Shader module preprocessing failed - " << preProcessed.GetErrorMessage() << std::endl;

	std::string ppData(preProcessed.begin());
	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(ppData.data(), ppData.size(), shaderc_glsl_infer_from_source, id.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success)
		std::cerr << "Shader module compilation failed - " << module.GetErrorMessage() << std::endl;

	std::vector<uint32_t> spirv = { module.cbegin(), module.cend() };

	//Create shader module:

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = reinterpret_cast<const uint32_t*>(spirv.data());	// The default allocator from std::vector ensures that the data satisfies the alignment requirements of `uint32_t`.

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(e.c.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module!");

	// Create and save shader object
	loadedShaders.emplace(loadedShaders.end(), e, id, shaderModule);	//loadedShaders.push_back(Shader(e, id, shaderModule));
	return (--loadedShaders.end());
}


// TEXTURE --------------------------------------------------------

Texture::Texture(VulkanEnvironment& e, const std::string& id, VkImage textureImage, VkDeviceMemory textureImageMemory, VkImageView textureImageView, VkSampler textureSampler)
	: e(e), id(id), counter(0), textureImage(textureImage), textureImageMemory(textureImageMemory), textureImageView(textureImageView), textureSampler(textureSampler) { }

Texture::~Texture()
{
	vkDestroySampler(e.c.device, textureSampler, nullptr);
	vkDestroyImage(e.c.device, textureImage, nullptr);
	vkDestroyImageView(e.c.device, textureImageView, nullptr);
	vkFreeMemory(e.c.device, textureImageMemory, nullptr);
}

TextureInfo::TextureInfo(std::string filePath, VkFormat imageFormat , VkSamplerAddressMode addressMode)
	: filePath(filePath), pixels(nullptr), texWidth(0), texHeight(0), e(nullptr), mipLevels(0), id(filePath), imageFormat(imageFormat), addressMode(addressMode) { }

TextureInfo::TextureInfo(unsigned char* pixels, int texWidth, int texHeight, std::string id, VkFormat imageFormat, VkSamplerAddressMode addressMode)
	: filePath(""), pixels(nullptr), texWidth(texWidth), texHeight(texHeight), e(nullptr), mipLevels(0), id(id), imageFormat(imageFormat), addressMode(addressMode)
{
	size_t size = sizeof(float) * texHeight * texWidth;
	this->pixels = new unsigned char[size];
	std::copy(pixels, pixels + size, this->pixels);		//memcpy(this->pixels, pixels, size);
}

TextureInfo& TextureInfo::operator=(const TextureInfo& obj)
{
	if (pixels)
	{
		size_t size = sizeof(float) * obj.texHeight * obj.texWidth;
		pixels = new unsigned char[size];
		std::copy(obj.pixels, obj.pixels + size, this->pixels);		//memcpy(this->pixels, pixels, size);
	}
	else pixels = nullptr;

	filePath	= obj.filePath;
	texWidth	= obj.texWidth;
	texHeight	= obj.texHeight;
	e			= obj.e;
	mipLevels	= obj.mipLevels;
	id			= obj.id;
	imageFormat = obj.imageFormat;
	addressMode = obj.addressMode;

	return *this;
}

TextureInfo::~TextureInfo() 
{
	if (!pixels) delete pixels;
}

std::list<Texture>::iterator TextureInfo::loadTexture(VulkanEnvironment& e, std::list<Texture>& loadedTextures)
{
	#ifdef DEBUG_RESOURCES
		std::cout << typeid(*this).name() << "::" << __func__ << ": " << this->id << std::endl;
	#endif

	this->e = &e;

	// Look for it in loadedShaders
	for (auto i = loadedTextures.begin(); i != loadedTextures.end(); i++)
		if (i->id == id) return i;
	
	// Load an image
	if (filePath.size())	// from file
	{
		int texChannels;
		pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);	// Returns a pointer to an array of pixel values. STBI_rgb_alpha forces the image to be loaded with an alpha channel, even if it doesn't have one.
		if (!pixels) throw std::runtime_error("Failed to load texture image!");
	}
	
	// Get arguments for creating the texture object
	std::pair<VkImage, VkDeviceMemory> image = createTextureImage();
	VkImageView textureImageView             = createTextureImageView(std::get<VkImage>(image));
	VkSampler textureSampler                 = createTextureSampler();

	// Create and save texture object
	loadedTextures.emplace(loadedTextures.end(), e, id, std::get<VkImage>(image), std::get<VkDeviceMemory>(image), textureImageView, textureSampler);		//loadedTextures.push_back(texture);
	return (--loadedTextures.end());
}

std::pair<VkImage, VkDeviceMemory> TextureInfo::createTextureImage()
{
	#ifdef DEBUG_RESOURCES
		std::cout << "   " << __func__ << std::endl;
	#endif

	VkDeviceSize imageSize = texWidth * texHeight * 4;												// 4 bytes per rgba pixel
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;	// Calculate the number levels (mipmaps)

	// Create a staging buffer (temporary buffer in host visible memory so that we can use vkMapMemory and copy the pixels to it)
	VkBuffer	   stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	
	createBuffer(
		e,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);
	
	// Copy directly the pixel values from the image we loaded to the staging-buffer.
	void* data;
	vkMapMemory(e->c.device, stagingBufferMemory, 0, imageSize, 0, &data);	// vkMapMemory retrieves a host virtual address pointer (data) to a region of a mappable memory object (stagingBufferMemory). We have to provide the logical device that owns the memory (e.device).
	memcpy(data, pixels, static_cast<size_t>(imageSize));					// Copies a number of bytes (imageSize) from a source (pixels) to a destination (data).
	vkUnmapMemory(e->c.device, stagingBufferMemory);						// Unmap a previously mapped memory object (stagingBufferMemory).
	
	stbi_image_free(pixels);	// Clean up the original pixel array
	
	// Create the texture image
	VkImage			textureImage;
	VkDeviceMemory	textureImageMemory;

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
	vkDestroyBuffer(e->c.device, stagingBuffer, nullptr);
	vkFreeMemory(e->c.device, stagingBufferMemory, nullptr);
	
	return std::pair(textureImage, textureImageMemory);
}

VkImageView TextureInfo::createTextureImageView(VkImage textureImage)
{
	#ifdef DEBUG_RESOURCES
		std::cout << "   " << __func__ << std::endl;
	#endif

	return e->createImageView(textureImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

VkSampler TextureInfo::createTextureSampler()
{
	#ifdef DEBUG_RESOURCES
		std::cout << "   " << __func__ << std::endl;
	#endif

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;					// How to interpolate texels that are magnified (oversampling) or ...
	samplerInfo.minFilter = VK_FILTER_LINEAR;					// ... minified (undersampling). Choices: VK_FILTER_NEAREST, VK_FILTER_LINEAR
	samplerInfo.addressModeU = addressMode;						// Addressing mode per axis (what happens when going beyond the image dimensions). In texture space coordinates, XYZ are UVW. Available values: VK_SAMPLER_ADDRESS_MODE_ ... REPEAT (repeat the texture), MIRRORED_REPEAT (like repeat, but inverts coordinates to mirror the image), CLAMP_TO_EDGE (take the color of the closest edge), MIRROR_CLAMP_TO_EDGE (like clamp to edge, but taking the opposite edge), CLAMP_TO_BORDER (return solid color).
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;

	if (e->c.supportsAF)		// If anisotropic filtering is available (see isDeviceSuitable) <<<<<
	{
		samplerInfo.anisotropyEnable = VK_TRUE;							// Specify if anisotropic filtering should be used
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(e->c.physicalDevice, &properties);
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

	VkSampler textureSampler;
	if (vkCreateSampler(e->c.device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture sampler!");
	return textureSampler;

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

void TextureInfo::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
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

void TextureInfo::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	// Check if the image format supports linear blitting. We are using vkCmdBlitImage, but it's not guaranteed to be supported on all platforms bacause it requires our texture image format to support linear filtering, so we check it with vkGetPhysicalDeviceFormatProperties.
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(e->c.physicalDevice, imageFormat, &formatProperties);
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









Texture0::Texture0(std::string path, VkFormat imageFormat, VkSamplerAddressMode addressMode)
	: path(path), e(nullptr), imageFormat(imageFormat), addressMode(addressMode), texWidth(0), texHeight(0), texChannels(0), pixels(nullptr), fullyConstructed(false), mipLevels(0)
{
	#ifdef DEBUG_RESOURCES
		std::cout << typeid(*this).name() << "::" << __func__ << ": " << this->path << std::endl;
	#endif
		
	//copyCString(this->path, path);	// where path is const char*
}

Texture0::Texture0(unsigned char* pixels, int texWidth, int texHeight, VkFormat imageFormat, VkSamplerAddressMode addressMode)
	: path(), e(nullptr), imageFormat(imageFormat), addressMode(addressMode), texWidth(texWidth), texHeight(texHeight), texChannels(0), pixels(nullptr), fullyConstructed(false), mipLevels(0)
{
	#ifdef DEBUG_RESOURCES
		std::cout << typeid(*this).name() << "::" << __func__ << ": Texture from in-code data" << std::endl;
	#endif

	this->pixels = new unsigned char[4 * texHeight * texWidth];	// Copy texture to member texture
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
Texture0::~Texture0() 
{ 
	//if(path) delete[] path;

	if (e)
	{
		vkDestroySampler(e->c.device, textureSampler, nullptr);
		vkDestroyImage(e->c.device, textureImage, nullptr);
		vkDestroyImageView(e->c.device, textureImageView, nullptr);
		vkFreeMemory(e->c.device, textureImageMemory, nullptr);
	}
}

void Texture0::loadAndCreateTexture(VulkanEnvironment* e)
{
	#ifdef DEBUG_RESOURCES
		if (path.size())
			std::cout << typeid(*this).name() << "::" << __func__ << " (begin) (" << path.size() << "): " << path << std::endl;
		else
			std::cout << typeid(*this).name() << "::" << __func__ << " (begin): " << "In-code generated texture" << std::endl;
	#endif

	this->e = e;
	
	createTextureImage();
	createTextureImageView();
	createTextureSampler();

	//fullyConstructed = true;

	#ifdef DEBUG_RESOURCES
		if (path.size())
			std::cout << typeid(*this).name() << "::" << __func__ << " (end): " << path << std::endl;
		else
			std::cout << typeid(*this).name() << "::" << __func__ << " (end): " << "In-code generated texture" << std::endl;
	#endif
}

// (15)
void Texture0::createTextureImage()
{
	#ifdef DEBUG_RESOURCES
		std::cout << "   " << __func__ << std::endl;
	#endif

	// Load an image (usually, the most expensive process)
	if (path.size())	// data from file
	{
		pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);		// Returns a pointer to an array of pixel values. STBI_rgb_alpha forces the image to be loaded with an alpha channel, even if it doesn't have one.
		if (!pixels) throw std::runtime_error("Failed to load texture image!");
	}
	else { }			// data from code

	VkDeviceSize imageSize = texWidth * texHeight * 4;												// 4 bytes per rgba pixel
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;	// Calculate the number levels (mipmaps)
	
	// Create a staging buffer (temporary buffer in host visible memory so that we can use vkMapMemory and copy the pixels to it)
	VkBuffer	   stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(
		e,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// Copy directly the pixel values from the image we loaded to the staging-buffer.
	void* data;
	vkMapMemory(e->c.device, stagingBufferMemory, 0, imageSize, 0, &data);	// vkMapMemory retrieves a host virtual address pointer (data) to a region of a mappable memory object (stagingBufferMemory). We have to provide the logical device that owns the memory (e.device).
	memcpy(data, pixels, static_cast<size_t>(imageSize));					// Copies a number of bytes (imageSize) from a source (pixels) to a destination (data).
	vkUnmapMemory(e->c.device, stagingBufferMemory);						// Unmap a previously mapped memory object (stagingBufferMemory).
	
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
	vkDestroyBuffer(e->c.device, stagingBuffer, nullptr);
	vkFreeMemory(e->c.device, stagingBufferMemory, nullptr);
}

void Texture0::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
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

void Texture0::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	// Check if the image format supports linear blitting. We are using vkCmdBlitImage, but it's not guaranteed to be supported on all platforms bacause it requires our texture image format to support linear filtering, so we check it with vkGetPhysicalDeviceFormatProperties.
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(e->c.physicalDevice, imageFormat, &formatProperties);
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
void Texture0::createTextureImageView()
{
	#ifdef DEBUG_RESOURCES
		std::cout << "   " << __func__ << std::endl;
	#endif

	textureImageView = e->createImageView(textureImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

// (17)
void Texture0::createTextureSampler()
{
	#ifdef DEBUG_RESOURCES
		std::cout << "   " << __func__ << std::endl;
	#endif

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;					// How to interpolate texels that are magnified (oversampling) or ...
	samplerInfo.minFilter = VK_FILTER_LINEAR;					// ... minified (undersampling). Choices: VK_FILTER_NEAREST, VK_FILTER_LINEAR
	samplerInfo.addressModeU = addressMode;						// Addressing mode per axis (what happens when going beyond the image dimensions). In texture space coordinates, XYZ are UVW. Available values: VK_SAMPLER_ADDRESS_MODE_ ... REPEAT (repeat the texture), MIRRORED_REPEAT (like repeat, but inverts coordinates to mirror the image), CLAMP_TO_EDGE (take the color of the closest edge), MIRROR_CLAMP_TO_EDGE (like clamp to edge, but taking the opposite edge), CLAMP_TO_BORDER (return solid color).
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;

	if (e->c.supportsAF)		// If anisotropic filtering is available (see isDeviceSuitable) <<<<<
	{
		samplerInfo.anisotropyEnable = VK_TRUE;							// Specify if anisotropic filtering should be used
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(e->c.physicalDevice, &properties);
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

	if (vkCreateSampler(e->c.device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
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

	// Look up
	float heightRatio = (2400.f - planetRadius) / (atmosphereRadius - planetRadius);
	float angleRatio = angle / angleRange;
	unsigned i = (heightSteps - 1) * heightRatio;
	unsigned j = (angleSteps - 1) * angleRatio;
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
