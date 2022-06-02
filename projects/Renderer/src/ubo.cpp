
#include <iostream>

#include "ubo.hpp"
#include "commons.hpp"


// Dynamic Uniform Buffer Objects -----------------------------------------------------------------

size_t UniformAlignment	= 16;
size_t MMsize			= sizeof(glm::mat4);
size_t VMsize			= sizeof(glm::mat4);
size_t PMsize			= sizeof(glm::mat4);
size_t MMNsize			= sizeof(glm::mat4);		// vec3
size_t lightSize		= sizeof(Light);
size_t vec4size			= sizeof(glm::vec4);
size_t materialSize		= sizeof(Material);

UBOconfig::UBOconfig(size_t dynBlocksCount, size_t size1, size_t size2, size_t size3, size_t size4, size_t size5)
{
	this->dynBlocksCount = dynBlocksCount;

	if (size1) attribsSize.push_back(size1);
	if (size2) attribsSize.push_back(size2);
	if (size3) attribsSize.push_back(size3);
	if (size4) attribsSize.push_back(size4);
	if (size5) attribsSize.push_back(size5);

	// Apply alignment (16 bytes) for each variable in the UBO. If any of these variables is a struct, its members should be aligned too (16 bytes).
	//if (size1) attribsSize.push_back(size1 % UniformAlignment ? size1 + UniformAlignment - (size1 % UniformAlignment) : size1);
	//if (size2) attribsSize.push_back(size2 % UniformAlignment ? size2 + UniformAlignment - (size2 % UniformAlignment) : size2);
	//if (size3) attribsSize.push_back(size3 % UniformAlignment ? size3 + UniformAlignment - (size3 % UniformAlignment) : size3);
	//if (size4) attribsSize.push_back(size4 % UniformAlignment ? size4 + UniformAlignment - (size4 % UniformAlignment) : size4);
	//if (size5) attribsSize.push_back(size5 % UniformAlignment ? size5 + UniformAlignment - (size5 % UniformAlignment) : size5);
}

/// Constructor. Computes sizes (range, totalBytes) and allocates buffers (ubo, dynamicOffsets).
UBO::UBO(VulkanEnvironment& e, const UBOconfig& config, VkDeviceSize minUBOffsetAlignment)
	: e(e), range(0)
{	
	attribsSize = config.attribsSize;

	// range
	size_t usefulUBOsize = 0;						// Section of the range that will be actually used(example: 3)
	for (size_t i = 0; i < attribsSize.size(); i++)
		usefulUBOsize += attribsSize[i];

	if (usefulUBOsize)
		range = minUBOffsetAlignment * (1 + usefulUBOsize / minUBOffsetAlignment);

	// dynBlocksCount, totalBytes, UBO, dynamicOffsets
	resizeUBO(config.dynBlocksCount);

	// uniformsOffsets
	uniformsOffsets.resize(attribsSize.size());
	if (uniformsOffsets.size()) uniformsOffsets[0] = 0;
	for (size_t i = 1; i < uniformsOffsets.size(); i++)
		uniformsOffsets[i] = uniformsOffsets[i - 1] + attribsSize[i - 1];
}

void UBO::resizeUBO(size_t newDynBlocksCount)// <<< what to do in modelData if uboType == 0
{	
	// dynBlocksCount
	dynBlocksCount = newDynBlocksCount;

	// totalBytes
	totalBytes = range * dynBlocksCount;

	// UBO
	ubo.resize(totalBytes);

	// dynamicOffsets
	dynamicOffsets.resize(dynBlocksCount);
	for (size_t i = 0; i < dynamicOffsets.size(); ++i)
		dynamicOffsets[i] = i * range;
}

// (21)
void UBO::createUniformBuffers()
{
	uniformBuffers.resize(e.swapChainImages.size());
	uniformBuffersMemory.resize(e.swapChainImages.size());
	
	//destroyUniformBuffers();		// Not required since Renderer calls this first

	if (range)
		for (size_t i = 0; i < e.swapChainImages.size(); i++)
			createBuffer(
				e,
				dynBlocksCount == 0 ? range : totalBytes,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniformBuffers[i],
				uniformBuffersMemory[i]);
}

void UBO::destroyUniformBuffers()
{
	if (range)
	{
		std::cout << "   UBOs" << std::endl;
		for (size_t i = 0; i < e.swapChainImages.size(); i++)
		{
			vkDestroyBuffer(e.device, uniformBuffers[i], nullptr);
			vkFreeMemory(e.device, uniformBuffersMemory[i], nullptr);
		}
	}
}

Material::Material(glm::vec3& diffuse, glm::vec3& specular, float shininess)
	: diffuse(diffuse), specular(specular), shininess(shininess) { }


// Light -----------------------------------------------------------------

Light::Light() { lightType = 0; }

void Light::turnOff() { lightType = 0; }

void Light::setDirectional(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
{
	this->lightType = 1;

	this->direction = direction;

	this->ambient = ambient;
	this->diffuse = diffuse;
	this->specular = specular;

	this->degree[0] = 1;
	this->degree[1] = 1;
	this->degree[2] = 1;
}

void Light::setPoint(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic)
{
	this->lightType = 2;

	this->position = position;

	this->ambient = ambient;
	this->diffuse = diffuse;
	this->specular = specular;

	this->degree[0] = constant;
	this->degree[1] = linear;
	this->degree[2] = quadratic;
}

void Light::setSpot(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic, float cutOff, float outerCutOff)
{
	this->lightType = 3;

	this->position = position;
	this->direction = direction;

	this->ambient = ambient;
	this->diffuse = diffuse;
	this->specular = specular;

	this->degree[0] = constant;
	this->degree[1] = linear;
	this->degree[2] = quadratic;

	this->cutOff[0] = cutOff;
	this->cutOff[1] = outerCutOff;
}
