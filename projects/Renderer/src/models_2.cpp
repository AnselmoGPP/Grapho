
#include <iostream>

#include "models_2.hpp"


//VertexType -----------------------------------------------------------------

VertexType::VertexType(size_t numP, size_t numC, size_t numT, size_t numN)
{ 
	vertexSize = numP * attribsSize[0] + numC * attribsSize[1] + numT * attribsSize[2] + numN * attribsSize[3];

	numEachAttrib[0] = numP;
	numEachAttrib[1] = numC;
	numEachAttrib[2] = numT;
	numEachAttrib[3] = numN;

	//std::cout << "Vertex size: " << vertexSize << std::endl;
	//std::cout << "   Attributes: " << numEachAttrib[0] << ", " << numEachAttrib[1] << ", " << numEachAttrib[2] << std::endl;
	//std::cout << "   Sizes: " << attribsSize[0] << ", " << attribsSize[1] << ", " << attribsSize[2] << std::endl;
}

VertexType::~VertexType() { }

VertexType& VertexType::operator=(const VertexType& obj)
{
	if (&obj == this) return *this;

	vertexSize = obj.vertexSize;
	numEachAttrib = obj.numEachAttrib;

	return *this;
}

VkVertexInputBindingDescription VertexType::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;									// Index of the binding in the array of bindings. We have a single array, so we only have one binding.
	bindingDescription.stride = vertexSize;							// Number of bytes from one entry to the next.
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// VK_VERTEX_INPUT_RATE_ ... VERTEX, INSTANCE (move to the next data entry after each vertex or instance).

	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> VertexType::getAttributeDescriptions()
{
	VkVertexInputAttributeDescription vertexAttrib;
	uint32_t location = 0;
	uint32_t offset = 0;

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

	// POSITION
	for (size_t i = 0; i < numEachAttrib[0]; i++)
	{
		vertexAttrib.binding = 0;							// From which binding the per-vertex data comes.
		vertexAttrib.location = location;					// Directive "location" of the input in the vertex shader.
		vertexAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;	// Type of data for the attribute: VK_FORMAT_ ... R32_SFLOAT (float), R32G32_SFLOAT (vec2), R32G32B32_SFLOAT (vec3), R32G32B32A32_SFLOAT (vec4), R64_SFLOAT (64-bit double), R32G32B32A32_UINT (uvec4: 32-bit unsigned int), R32G32_SINT (ivec2: 32-bit signed int)...
		vertexAttrib.offset = offset;						// Number of bytes since the start of the per-vertex data to read from. // offsetof(VertexPCT, pos);	

		location++;
		offset += attribsSize[0];
		attributeDescriptions.push_back(vertexAttrib);
	}

	// COLOR
	for (size_t i = 0; i < numEachAttrib[1]; i++)
	{
		vertexAttrib.binding = 0;
		vertexAttrib.location = location;
		vertexAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttrib.offset = offset;	// offsetof(VertexPCT, color);

		location++;
		offset += attribsSize[1];
		attributeDescriptions.push_back(vertexAttrib);
	}

	// TEXTURE COORDINATES
	for (size_t i = 0; i < numEachAttrib[2]; i++)
	{
		vertexAttrib.binding = 0;
		vertexAttrib.location = location;
		vertexAttrib.format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttrib.offset = offset;	// offsetof(VertexPCT, texCoord);

		location++;
		offset += attribsSize[2];
		attributeDescriptions.push_back(vertexAttrib);
	}

	// NORMALS
	for (size_t i = 0; i < numEachAttrib[3]; i++)
	{
		vertexAttrib.binding = 0;
		vertexAttrib.location = location;
		vertexAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttrib.offset = offset;

		location++;
		offset += attribsSize[3];
		attributeDescriptions.push_back(vertexAttrib);
	}

	return attributeDescriptions;
}


//VerteSet -----------------------------------------------------------------

VertexSet::VertexSet(VertexType vertexType)
	: capacity(8), numVertex(0), Vtype(vertexType)
{ 
	this->buffer = new char[capacity * vertexType.vertexSize];
}

VertexSet::VertexSet(VertexType vertexType, size_t numOfVertex, const void* buffer)
	: numVertex(numOfVertex), Vtype(vertexType)
{
	capacity = pow(2, 1 + (int)(log(numOfVertex)/log(2)));		// log b (M) = ln(M) / ln(b)
	this->buffer = new char[capacity * vertexType.vertexSize];
	std::memcpy(this->buffer, buffer, totalBytes());
}

VertexSet::~VertexSet() { delete[] buffer; };

VertexSet& VertexSet::operator=(const VertexSet& obj)
{
	if (&obj == this) return *this;

	numVertex = obj.numVertex;
	Vtype = obj.Vtype;
	capacity = obj.capacity;

	delete[] buffer;
	buffer = new char[capacity * Vtype.vertexSize];
	std::memcpy(buffer, obj.buffer, totalBytes());

	return *this;
}

size_t VertexSet::totalBytes() const { return numVertex * Vtype.vertexSize; }

size_t VertexSet::size() const { return numVertex; }

char* VertexSet::data() const { return buffer; }

void* VertexSet::getElement(size_t i) const { return &(buffer[i * Vtype.vertexSize]); }

void VertexSet::push_back(const void* element)
{
	// Resize buffer if required
	if (numVertex == capacity)
	{
		capacity *= 2;
		char* temp = new char[capacity * Vtype.vertexSize];
		std::memcpy(temp, buffer, totalBytes());
		delete[] buffer;
		buffer = temp;
	}

	std::memcpy(&buffer[totalBytes()], (char*)element, Vtype.vertexSize);
	numVertex++;
}


//Vertex PCT (Position, Color, Texture) -----------------------------------------------------------------

VertexPCT::VertexPCT(glm::vec3 vertex, glm::vec3 vertexColor, glm::vec2 textureCoordinates)
	: pos(vertex), color(vertexColor), texCoord(textureCoordinates) { }

VkVertexInputBindingDescription VertexPCT::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;							// Index of the binding in the array of bindings. We have a single array, so we only have one binding.
	bindingDescription.stride = sizeof(VertexPCT);			// Number of bytes from one entry to the next.
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	// VK_VERTEX_INPUT_RATE_ ... VERTEX, INSTANCE (move to the next data entry after each vertex or instance).

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VertexPCT::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;							// From which binding the per-vertex data comes.
	attributeDescriptions[0].location = 0;							// Directive "location" of the input in the vertex shader.
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Type of data for the attribute: VK_FORMAT_ ... R32_SFLOAT (float), R32G32_SFLOAT (vec2), R32G32B32_SFLOAT (vec3), R32G32B32A32_SFLOAT (vec4), R64_SFLOAT (64-bit double), R32G32B32A32_UINT (uvec4: 32-bit unsigned int), R32G32_SINT (ivec2: 32-bit signed int)...
	attributeDescriptions[0].offset = offsetof(VertexPCT, pos);		// Number of bytes since the start of the per-vertex data to read from.

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(VertexPCT, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(VertexPCT, texCoord);

	return attributeDescriptions;
}

bool VertexPCT::operator==(const VertexPCT& other) const {
	return	pos == other.pos &&
		color == other.color &&
		texCoord == other.texCoord;
}

size_t std::hash<VertexPCT>::operator()(VertexPCT const& vertex) const
{
	return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
}


//Vertex PC (Position, Color) -----------------------------------------------------------------

VertexPC::VertexPC(glm::vec3 vertex, glm::vec3 vertexColor)
	: pos(vertex), color(vertexColor) { }

VkVertexInputBindingDescription VertexPC::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;							// Index of the binding in the array of bindings. We have a single array, so we only have one binding.
	bindingDescription.stride = sizeof(VertexPC);			// Number of bytes from one entry to the next.
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	// VK_VERTEX_INPUT_RATE_ ... VERTEX, INSTANCE (move to the next data entry after each vertex or instance).

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VertexPC::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;							// From which binding the per-vertex data comes.
	attributeDescriptions[0].location = 0;							// Directive "location" of the input in the vertex shader.
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Type of data for the attribute: VK_FORMAT_ ... R32_SFLOAT (float), R32G32_SFLOAT (vec2), R32G32B32_SFLOAT (vec3), R32G32B32A32_SFLOAT (vec4), R64_SFLOAT (64-bit double), R32G32B32A32_UINT (uvec4: 32-bit unsigned int), R32G32_SINT (ivec2: 32-bit signed int)...
	attributeDescriptions[0].offset = offsetof(VertexPC, pos);		// Number of bytes since the start of the per-vertex data to read from.

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(VertexPC, color);

	return attributeDescriptions;
}

bool VertexPC::operator==(const VertexPC& other) const {
	return	pos == other.pos &&
			color == other.color;
}

size_t std::hash<VertexPC>::operator()(VertexPC const& vertex) const
{
	return ( (hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1 );
}


//Vertex PT (Position, Texture) -----------------------------------------------------------------

VertexPT::VertexPT(glm::vec3 vertex, glm::vec2 textureCoordinates)
	: pos(vertex), texCoord(textureCoordinates) { }

VkVertexInputBindingDescription VertexPT::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;							// Index of the binding in the array of bindings. We have a single array, so we only have one binding.
	bindingDescription.stride = sizeof(VertexPT);			// Number of bytes from one entry to the next.
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	// VK_VERTEX_INPUT_RATE_ ... VERTEX, INSTANCE (move to the next data entry after each vertex or instance).

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VertexPT::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;							// From which binding the per-vertex data comes.
	attributeDescriptions[0].location = 0;							// Directive "location" of the input in the vertex shader.
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Type of data for the attribute: VK_FORMAT_ ... R32_SFLOAT (float), R32G32_SFLOAT (vec2), R32G32B32_SFLOAT (vec3), R32G32B32A32_SFLOAT (vec4), R64_SFLOAT (64-bit double), R32G32B32A32_UINT (uvec4: 32-bit unsigned int), R32G32_SINT (ivec2: 32-bit signed int)...
	attributeDescriptions[0].offset = offsetof(VertexPT, pos);		// Number of bytes since the start of the per-vertex data to read from.

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(VertexPT, texCoord);

	return attributeDescriptions;
}

bool VertexPT::operator==(const VertexPT& other) const {
	return	pos == other.pos &&
		texCoord == other.texCoord;
}

size_t std::hash<VertexPT>::operator()(VertexPT const& vertex) const
{
	return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1);
}


// Dynamic Uniform Buffer Objects -----------------------------------------------------------------

UBOtype::UBOtype(size_t numM, size_t numV, size_t numP, size_t numMN)
{
	numEachAttrib[0] = numM;
	numEachAttrib[1] = numV;
	numEachAttrib[2] = numP;
	numEachAttrib[3] = numMN;
}

UBOdynamic::UBOdynamic(size_t UBOcount, const UBOtype& uboType, VkDeviceSize minUBOffsetAlignment)
	: count(0), dirtyCount(0)
{
	// Get amount of each attribute per dynamic UBO
	for(size_t i = 0; i < numEachAttrib.size(); i++)
		numEachAttrib[i] = uboType.numEachAttrib[i];

	// Get range
	VkDeviceSize usefulUBOsize = 0;			// Section of the range that will be actually used (example: 3)
	for (size_t i = 0; i < numEachAttrib.size(); i++)
		usefulUBOsize += attribsSize[i] * numEachAttrib[i];

	range = minUBOffsetAlignment * (1 + usefulUBOsize / minUBOffsetAlignment);

	// Resize buffers
	resize(UBOcount);
}

UBOdynamic& UBOdynamic::operator = (const UBOdynamic& obj)
{
	count = obj.count;
	dirtyCount = obj.dirtyCount;
	range = obj.range;
	totalBytes = obj.totalBytes;
	ubo.resize(totalBytes);

	return *this;
}

void UBOdynamic::resize(size_t newCount)
{	
	size_t oldCount = count;

	count = dirtyCount = newCount;
	totalBytes = newCount * range;

	ubo.resize(totalBytes);
	dynamicOffsets.resize(newCount);

	if (newCount > oldCount)
	{
		glm::mat4 defaultM;
		defaultM = glm::mat4(1.0f);
		//defaultM = glm::translate(defaultM, glm::vec3(0.0f, 0.0f, 0.0f));//<<< comment this
		//defaultM = glm::rotate(defaultM, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//defaultM = glm::scale(defaultM, glm::vec3(1.0f, 1.0f, 1.0f));

		glm::mat3 defaultMNor = glm::mat3(glm::transpose(glm::inverse(defaultM)));

		for (size_t i = oldCount; i < newCount; ++i)
		{
			if (numEachAttrib[0]) setModel(i, defaultM);
			if (numEachAttrib[3]) setMNor (i, defaultMNor);
			dynamicOffsets[i] = i * range;
		}
	}
}

void UBOdynamic::dirtyResize(size_t newCount) 
{
	ubo.resize(newCount * range);
	dirtyCount = newCount;
}

void UBOdynamic::setModel(size_t position, const glm::mat4& matrix)
{
	glm::mat4* destination = (glm::mat4*)&ubo.data()[position * range];
	*destination = matrix;					// Equivalent to:   memcpy((void*)original, (void*)&matrix, sizeof(glm::mat4));
}

void UBOdynamic::setView(size_t position, const glm::mat4& matrix)
{
	glm::mat4* destination = (glm::mat4*)&ubo.data()[position * range + attribsSize[0] * numEachAttrib[0]];
	*destination = matrix;
}

void UBOdynamic::setProj(size_t position, const glm::mat4& matrix)
{
	glm::mat4* destination = (glm::mat4*)&ubo.data()[position * range + attribsSize[0] * numEachAttrib[0] + attribsSize[1] * numEachAttrib[1]];
	*destination = matrix;
}

void UBOdynamic::setMNor(size_t position, const glm::mat3& matrix)
{
	glm::mat4* destination = (glm::mat4*)&ubo.data()[position * range + attribsSize[0] * numEachAttrib[0] + attribsSize[1] * numEachAttrib[1] + attribsSize[2] * numEachAttrib[2]];
	*destination = matrix;
}


// Textures -----------------------------------------------------------------

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(const char* path) : path(nullptr), e(nullptr)
{
	std::cout << "Texture constructor" << std::endl;
	copyCString(this->path, path);
	std::cout << path << std::endl;
}

Texture::Texture(const Texture& obj)
{
	std::cout << "Copy constructor" << std::endl;
	copyCString(this->path, obj.path);
	std::cout << path << std::endl;

	if (e)
	{
		this->e = obj.e;
		this->mipLevels = obj.mipLevels;
		this->textureImage = obj.textureImage;
		this->textureImageMemory = obj.textureImageMemory;
		this->textureImageView = obj.textureImageView;
		this->textureSampler = obj.textureSampler;
	}
}

Texture::~Texture() 
{ 
	delete[] path; 
	if (e)
	{
		vkDestroySampler(e->device, textureSampler, nullptr);
		vkDestroyImageView(e->device, textureImageView, nullptr);
		vkDestroyImage(e->device, textureImage, nullptr);
		vkFreeMemory(e->device, textureImageMemory, nullptr);
	}
}

void Texture::loadAndCreateTexture(VulkanEnvironment& e)
{
	std::cout << "Texture load 1" << std::endl;
	this->e = &e;
	std::cout << "Texture load 2" << std::endl;
	createTextureImage(); std::cout << "Texture load 3" << std::endl;
	createTextureImageView(); std::cout << "Texture load 4" << std::endl;
	createTextureSampler(); std::cout << "Texture load 5" << std::endl;
}

// (15)
/// Load a texture > Copy it to a buffer > Copy it to an image > Cleanup the buffer
void Texture::createTextureImage()
{
std::cout << "createTextureImage 1" << std::endl;
std::cout << path << std::endl;
	// Load an image (usually, the most expensive process)
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);		// Returns a pointer to an array of pixel values. STBI_rgb_alpha forces the image to be loaded with an alpha channel, even if it doesn't have one.
	if (!pixels)
		throw std::runtime_error("Failed to load texture image!");
std::cout << "createTextureImage 2" << std::endl;
	VkDeviceSize imageSize = texWidth * texHeight * 4;												// 4 bytes per rgba pixel
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;	// Calculate the number levels (mipmaps)

	// Create a staging buffer (temporary buffer in host visible memory so that we can use vkMapMemory and copy the pixels to it)
	VkBuffer	   stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// Copy directly the pixel values from the image we loaded to the staging-buffer.
	void* data;
	vkMapMemory(e->device, stagingBufferMemory, 0, imageSize, 0, &data);	// vkMapMemory retrieves a host virtual address pointer (data) to a region of a mappable memory object (stagingBufferMemory). We have to provide the logical device that owns the memory (e.device).
	memcpy(data, pixels, static_cast<size_t>(imageSize));				// Copies a number of bytes (imageSize) from a source (pixels) to a destination (data).
	vkUnmapMemory(e->device, stagingBufferMemory);						// Unmap a previously mapped memory object (stagingBufferMemory).
	
	stbi_image_free(pixels);	// Clean up the original pixel array

	// Create the texture image
	e->createImage(texWidth,
		texHeight,
		mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureImageMemory);
	
	// Copy the staging buffer to the texture image
	e->transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);					// Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));											// Execute the buffer to image copy operation
	// Transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
	// transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);	// To be able to start sampling from the texture image in the shader, we need one last transition to prepare it for shader access
	generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

	// Cleanup the staging buffer and its memory
	vkDestroyBuffer(e->device, stagingBuffer, nullptr);
	vkFreeMemory(e->device, stagingBufferMemory, nullptr);
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
	vkCmdCopyBufferToImage(commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,			// Layout the image is currently using
		1,
		&region);

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
	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	e->endSingleTimeCommands(commandBuffer);
}

// (16)
void Texture::createTextureImageView()
{
	textureImageView = e->createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

// (17)
void Texture::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;					// How to interpolate texels that are magnified (oversampling) or ...
	samplerInfo.minFilter = VK_FILTER_LINEAR;					// ... minified (undersampling). Choices: VK_FILTER_NEAREST, VK_FILTER_LINEAR
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// Addressing mode per axis (what happens when going beyond the image dimensions). In texture space coordinates, XYZ are UVW. Available values: VK_SAMPLER_ADDRESS_MODE_ ... REPEAT (repeat the texture), MIRRORED_REPEAT (like repeat, but inverts coordinates to mirror the image), CLAMP_TO_EDGE (take the color of the closest edge), MIRROR_CLAMP_TO_EDGE (like clamp to edge, but taking the opposite edge), CLAMP_TO_BORDER (return solid color).
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	if (1)		// If anisotropic filtering is available (see isDeviceSuitable) <<<<<
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

void Texture::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	// Create buffer.
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;									// For multiple purposes use a bitwise or.
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Like images in the swap chain, buffers can also be owned by a specific queue family or be shared between multiple at the same time. Since the buffer will only be used from the graphics queue, we use EXCLUSIVE.
	bufferInfo.flags = 0;										// Used to configure sparse buffer memory.

	if (vkCreateBuffer(e->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)	// vkCreateBuffer creates a new buffer object and returns it to a pointer to a VkBuffer provided by the caller.
		throw std::runtime_error("Failed to create buffer!");

	// Get buffer requirements.
	VkMemoryRequirements memRequirements;		// Members: size (amount of memory in bytes. May differ from bufferInfo.size), alignment (offset in bytes where the buffer begins in the allocated region. Depends on bufferInfo.usage and bufferInfo.flags), memoryTypeBits (bit field of the memory types that are suitable for the buffer).
	vkGetBufferMemoryRequirements(e->device, buffer, &memRequirements);

	// Allocate memory for the buffer.
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = e->findMemoryType(memRequirements.memoryTypeBits, properties);		// Properties parameter: We need to be able to write our vertex data to that memory. The properties define special features of the memory, like being able to map it so we can write to it from the CPU.

	if (vkAllocateMemory(e->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate buffer memory!");

	vkBindBufferMemory(e->device, buffer, bufferMemory, 0);	// Associate this memory with the buffer. If the offset (4th parameter) is non-zero, it's required to be divisible by memRequirements.alignment.
}

void Texture::copyCString(const char*& destination, const char* source)
{
	size_t siz = strlen(source) + 1;
	char* address = new char[siz];
	strncpy(address, source, siz);
	destination = address;
}
