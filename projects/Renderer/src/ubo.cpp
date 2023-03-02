
#include <iostream>

#include "ubo.hpp"
#include "commons.hpp"


// Dynamic Uniform Buffer Objects -----------------------------------------------------------------

size_t UniformAlignment	= 16;
size_t vec3size			= sizeof(glm::vec3);
size_t vec4size			= sizeof(glm::vec4);
size_t ivec4size		= sizeof(glm::ivec4);
size_t mat4size			= sizeof(glm::mat4);
size_t materialSize		= sizeof(Material);


/// Constructor. Computes sizes (range, totalBytes) and allocates buffers (ubo, dynamicOffsets).
UBO::UBO(VulkanEnvironment* e, size_t numDynUBOs, size_t dynUBOsize, VkDeviceSize minUBOffsetAlignment)
	: e(e), numDynUBOs(numDynUBOs), range(0)
{	
	if (dynUBOsize)
		range = minUBOffsetAlignment * (1 + dynUBOsize / minUBOffsetAlignment);

	totalBytes = range * numDynUBOs;
	ubo.resize(totalBytes);

	dynamicOffsets.resize(numDynUBOs);
	for (size_t i = 0; i < numDynUBOs; i++)
		dynamicOffsets[i] = i * range;
}

uint8_t* UBO::getUBOptr(size_t dynUBO) { return ubo.data() + dynUBO * range; }

void UBO::resizeUBO(size_t newNumDynUBOs)// <<< what to do in modelData if uboType == 0
{	
	numDynUBOs = newNumDynUBOs;

	totalBytes = range * numDynUBOs;

	ubo.resize(totalBytes);

	size_t oldSize = dynamicOffsets.size();
	dynamicOffsets.resize(numDynUBOs);
	for (size_t i = oldSize; i < numDynUBOs; i++)
		dynamicOffsets[i] = i * range;
}

// (21)
void UBO::createUniformBuffers()
{
	uniformBuffers.resize(e->swapChain.images.size());
	uniformBuffersMemory.resize(e->swapChain.images.size());
	
	//destroyUniformBuffers();		// Not required since Renderer calls this first

	if (range)
		for (size_t i = 0; i < e->swapChain.images.size(); i++)
			createBuffer(
				e,
				numDynUBOs == 0 ? range : totalBytes,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniformBuffers[i],
				uniformBuffersMemory[i]);
}

void UBO::destroyUniformBuffers()
{
	if (range)
	{
		for (size_t i = 0; i < e->swapChain.images.size(); i++)
		{
			vkDestroyBuffer(e->c.device, uniformBuffers[i], nullptr);
			vkFreeMemory(e->c.device, uniformBuffersMemory[i], nullptr);
		}
	}
}

Material::Material(glm::vec3& diffuse, glm::vec3& specular, float shininess)
	: diffuse(diffuse), specular(specular), shininess(shininess) { }


// LightSet -------------------------------------------------------------

LightSet::LightSet(int numLights)
	: numLights(numLights), posDirBytes(numLights * sizeof(LightPosDir)), propsBytes(numLights * sizeof(LightProps))
{
	this->posDir = new LightPosDir[numLights];
	this->props = new LightProps[numLights];

	for (size_t i = 0; i < numLights; i++)
		props[i].type = 0;

	if (numLights < 0) numLights = 0;
}

LightSet::~LightSet()
{
	delete[] posDir;
	delete[] props;
}

void LightSet::turnOff(size_t index) { props[index].type = 0; }

void LightSet::setDirectional(size_t index, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
{
	props[index].type = 1;
	posDir[index].direction = direction;

	props[index].ambient = ambient;
	props[index].diffuse = diffuse;
	props[index].specular = specular;
}

void LightSet::setPoint(size_t index, glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic)
{
	posDir[index].position = position;

	props[index].type = 2;

	props[index].ambient = ambient;
	props[index].diffuse = diffuse;
	props[index].specular = specular;

	props[index].degree.x = constant;
	props[index].degree.y = linear;
	props[index].degree.z = quadratic;
}

void LightSet::setSpot(size_t index, glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic, float cutOff, float outerCutOff)
{
	posDir[index].position = position;
	posDir[index].direction = direction;

	props[index].type = 3;

	props[index].ambient = ambient;
	props[index].diffuse = diffuse;
	props[index].specular = specular;

	props[index].degree.x = constant;
	props[index].degree.y = linear;
	props[index].degree.z = quadratic;

	props[index].cutOff.x = cutOff;
	props[index].cutOff.y = outerCutOff;
}


