
#include <iostream>

#include "vertex.hpp"

//VertexType -----------------------------------------------------------------

const std::array<size_t, 4> VertexType::attribsSize = { sizeof(glm::vec3), sizeof(glm::vec3), sizeof(glm::vec2), sizeof(glm::vec3) };

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

VertexType::VertexType(const VertexType& obj)
{
	numEachAttrib[0] = obj.numEachAttrib[0];
	numEachAttrib[1] = obj.numEachAttrib[1];
	numEachAttrib[2] = obj.numEachAttrib[2];
	numEachAttrib[3] = obj.numEachAttrib[3];

	vertexSize = numEachAttrib[0] * attribsSize[0] + numEachAttrib[1] * attribsSize[1] + numEachAttrib[2] * attribsSize[2] + numEachAttrib[3] * attribsSize[3];
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

void VertexSet::reset(VertexType vertexType, size_t numOfVertex, const void* buffer)
{
	delete[] this->buffer;

	Vtype = vertexType;
	numVertex = numOfVertex;

	capacity = pow(2, 1 + (int)(log(numOfVertex) / log(2)));		// log b (M) = ln(M) / ln(b)
	this->buffer = new char[capacity * vertexType.vertexSize];
	std::memcpy(this->buffer, buffer, totalBytes());
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
