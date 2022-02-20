
#include <iostream>

#include "ubo.hpp"
#include "commons.hpp"


// Dynamic Uniform Buffer Objects -----------------------------------------------------------------

const std::array<size_t, 5> UBOtype::attribsSize = { sizeof(glm::mat4), sizeof(glm::mat4), sizeof(glm::mat4), sizeof(glm::mat3) + 12, sizeof(Light) };	// 64, 64, 64, 36+12, 176 (including padding for getting alignment with 16 bits)

UBOtype::UBOtype(size_t numM, size_t numV, size_t numP, size_t numMN, size_t numLights)
{
	numEachAttrib[0] = numM;
	numEachAttrib[1] = numV;
	numEachAttrib[2] = numP;
	numEachAttrib[3] = numMN;
	numEachAttrib[4] = numLights;
}

/// Constructor. Computes sizes (range, totalBytes) and allocates buffers (ubo, dynamicOffsets).
UBO::UBO(VulkanEnvironment& e, size_t dynUBOcount, const UBOtype& uboType, VkDeviceSize minUBOffsetAlignment)
	: count(dynUBOcount), dirtyCount(dynUBOcount), range(0), e(e)
{
	//for(size_t i = 0; i < numEachAttrib.size(); i++)
	//	numEachAttrib[i] = uboType.numEachAttrib[i];

	numEachAttrib = uboType.numEachAttrib;

	size_t usefulUBOsize = 0;					///< Section of the range that will be actually used(example: 3)
	for (size_t i = 0; i < numEachAttrib.size(); i++)
		usefulUBOsize += UBOtype::attribsSize[i] * numEachAttrib[i];

	if (usefulUBOsize)
		range = minUBOffsetAlignment * (1 + usefulUBOsize / minUBOffsetAlignment);

	//resize(dynUBOcount);

	totalBytes = range * dynUBOcount;

	ubo.resize(totalBytes);
	dynamicOffsets.resize(dynUBOcount);

	// Initialize dynamicOffsets, MM, and MMN (Model Matrices & Model Matrices for Normals)
	glm::mat4 defaultMM(1.0f);
	glm::mat3 defaultMMN = glm::mat3(glm::transpose(glm::inverse(defaultMM)));

	for (size_t i = 0; i < dynUBOcount; ++i)
	{
		for (size_t j = 0; j < numEachAttrib[0]; ++j) setModelM(i, 0, defaultMM);
		for (size_t j = 0; j < numEachAttrib[3]; ++j) setMNorm (i, 0, defaultMMN);
		dynamicOffsets[i] = i * range;
	}
}

void UBO::resize(size_t newCount)// <<< what to do in modelData if uboType == 0
{	
	size_t oldCount = count;

	count = dirtyCount = newCount;
	totalBytes = newCount * range;

	ubo.resize(totalBytes);
	dynamicOffsets.resize(newCount);

	// Initialize the new Model matrices and Model matrices for normals
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
			for (size_t j = 0; j < numEachAttrib[0]; ++j) setModelM(i, 0, defaultM);
			for (size_t j = 0; j < numEachAttrib[3]; ++j) setMNorm (i, 0, defaultMNor);
			dynamicOffsets[i] = i * range;
		}
	}
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
				count == 0 ? range : totalBytes,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniformBuffers[i],
				uniformBuffersMemory[i]);
}

void UBO::destroyUniformBuffers()
{
	if (range)
		for (size_t i = 0; i < e.swapChainImages.size(); i++)
		{
			vkDestroyBuffer(e.device, uniformBuffers[i], nullptr);
			vkFreeMemory(e.device, uniformBuffersMemory[i], nullptr);
		}
}

void UBO::dirtyResize(size_t newCount)
{
	ubo.resize(newCount * range);
	dirtyCount = newCount;
}

size_t UBO::getPos(size_t dynUBO, size_t attribSet, size_t attrib)
{
	size_t pos = dynUBO * range;

	for (size_t i = 0; i < attribSet; ++i)
		pos += numEachAttrib[i] * UBOtype::attribsSize[i];

	pos += attrib * UBOtype::attribsSize[attribSet];

	return pos;
}

void UBO::setModelM(size_t posDyn, size_t attrib, const glm::mat4& matrix)
{
	glm::mat4* destination = (glm::mat4*)&ubo.data()[getPos(posDyn, 0, attrib)];
	*destination = matrix;					// Equivalent to:   memcpy((void*)original, (void*)&matrix, sizeof(glm::mat4));
}

void UBO::setViewM(size_t posDyn, size_t attrib, const glm::mat4& matrix)
{
	glm::mat4* destination = (glm::mat4*)&ubo.data()[getPos(posDyn, 1, attrib)];
	*destination = matrix;
}

void UBO::setProjM(size_t posDyn, size_t attrib, const glm::mat4& matrix)
{
	glm::mat4* destination = (glm::mat4*)&ubo.data()[getPos(posDyn, 2, attrib)];
	*destination = matrix;
}

void UBO::setMNorm(size_t posDyn, size_t attrib, const glm::mat3& matrix)
{
	glm::mat3* destination = (glm::mat3*)&ubo.data()[getPos(posDyn, 3, attrib)];
	*destination = matrix;
}

void UBO::setLight(size_t posDyn, size_t attrib, Light& light)
{
	Light* destination = (Light*)&ubo.data()[getPos(posDyn, 4, attrib)];
	*destination = light;
}

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
}

void Light::setPoint(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic)
{
	this->lightType = 2;
	this->position = position;
	this->ambient = ambient;
	this->diffuse = diffuse;
	this->specular = specular;
	this->constant = constant;
	this->linear = linear;
	this->quadratic = quadratic;
}

void Light::setSpot(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear, float quadratic, float cutOff, float outerCutOff)
{
	this->lightType = 3;
	this->position = position;
	this->direction = direction;
	this->ambient = ambient;
	this->diffuse = diffuse;
	this->specular = specular;
	this->constant = constant;
	this->linear = linear;
	this->quadratic = quadratic;
	this->cutOff = cutOff;
	this->outerCutOff = outerCutOff;
}
