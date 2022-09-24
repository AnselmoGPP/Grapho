#ifndef COMMONS_HPP
#define COMMONS_HPP

#include <list>

//#include <vulkan/vulkan.h>		// From LunarG SDK. Used for off-screen rendering
//define GLFW_INCLUDE_VULKAN		// Makes GLFW load the Vulkan header with it
//#include "GLFW/glfw3.h"

#include "environment.hpp"

typedef std::list<VkShaderModule>::iterator ShaderIter;

//extern std::vector< std::function<glm::mat4(float)> > room_MM;	// Store callbacks of type 'glm::mat4 callb(float a)'. Requires <functional> library

/// Creates a Vulkan buffer (VkBuffer and VkDeviceMemory).Used as friend in modelData, UBO and Texture. Used as friend in ModelData, Texture and UBO.
void createBuffer(VulkanEnvironment& e, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

/// Copy a C-style string in destination from source. Used in ModelData and Texture. Memory is allocated in "destination", remember to delete it when no needed anymore. 
void copyCString(const char*& destination, const char* source);

/// Read all of the bytes from the specified file and return them in a byte array managed by a std::vector.
void readFile(std::vector<char>& destination, const char* filename);

#endif