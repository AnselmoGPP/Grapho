
#include <fstream>
#include <iostream>

#include "commons.hpp"

//std::vector< std::function<glm::mat4(float)> > room_MM{ /*room1_MM, room2_MM, room3_MM, room4_MM*/ };

void createBuffer(VulkanEnvironment* e, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	// Create buffer.
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;									// For multiple purposes use a bitwise or.
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Like images in the swap chain, buffers can also be owned by a specific queue family or be shared between multiple at the same time. Since the buffer will only be used from the graphics queue, we use EXCLUSIVE.
	bufferInfo.flags = 0;										// Used to configure sparse buffer memory.

	if (vkCreateBuffer(e->c.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)	// vkCreateBuffer creates a new buffer object and returns it to a pointer to a VkBuffer provided by the caller.
		throw std::runtime_error("Failed to create buffer!");

	// Get buffer requirements.
	VkMemoryRequirements memRequirements;		// Members: size (amount of memory in bytes. May differ from bufferInfo.size), alignment (offset in bytes where the buffer begins in the allocated region. Depends on bufferInfo.usage and bufferInfo.flags), memoryTypeBits (bit field of the memory types that are suitable for the buffer).
	vkGetBufferMemoryRequirements(e->c.device, buffer, &memRequirements);
	
	// Allocate memory for the buffer.
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = e->findMemoryType(memRequirements.memoryTypeBits, properties);		// Properties parameter: We need to be able to write our vertex data to that memory. The properties define special features of the memory, like being able to map it so we can write to it from the CPU.

	if (vkAllocateMemory(e->c.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate buffer memory!");

	e->c.memAllocObjects++;

	vkBindBufferMemory(e->c.device, buffer, bufferMemory, 0);	// Associate this memory with the buffer. If the offset (4th parameter) is non-zero, it's required to be divisible by memRequirements.alignment.
}

void copyCString(const char*& destination, const char* source)
{
	size_t siz = strlen(source) + 1;
	char* address = new char[siz];
	strncpy(address, source, siz);
	destination = address;
}

// Read a file called <filename> and save its content in <destination>. Since a std::vector<char> is used, it may save garbage values at the end of the string.
void readFile(const char* filename, std::vector<char>& destination)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);	// Open file. // ate: Start reading at the end of the file  /  binary: Read file as binary file (avoid text transformations)
	if (!file.is_open())
		throw std::runtime_error("Failed to open file!");
	
	size_t fileSize = 0;
	fileSize = (size_t)file.tellg();
	destination.resize(fileSize);					// Allocate the buffer

	file.seekg(0);
	file.read(destination.data(), fileSize);		// Read data

	file.close();									// Close file
}

// Read a file called <filename> and save its content in <destination>.
void readFile(const char* filename, std::string& destination)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);	// Open file. // ate: Start reading at the end of the file  /  binary: Read file as binary file (avoid text transformations)
	if (!file.is_open())
		throw std::runtime_error("Failed to open file!");

	size_t fileSize = 0;
	fileSize = (size_t)file.tellg();
	destination.resize(fileSize);					// Allocate the buffer

	file.seekg(0);
	file.read(destination.data(), fileSize);		// Read data
	
	file.close();									// Close file
}


