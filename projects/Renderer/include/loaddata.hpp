#ifndef LOADDATA_HPP
#define LOADDATA_HPP

#include "vertex.hpp"

/**
	@class VertexLoader
	@brief Abstract data type used for defining how to load Vertices and Indices (from file, from user creation, from variable...) into our ModelData object.

	The user must define a subclass where to override loadVertex() (pure virtual method). User can override setDestination() (virtual) too, and create new methods and variables.
	setDestination() is called by Renderer's constructor. loadVertex() is called by Renderer::fullConstructor() (loading thread).
*/
class VertexLoader
{
protected:
	VertexType vertexType;
	VertexSet* destVertices;
	std::vector<uint16_t>* destIndices;

public:
	VertexLoader() : vertexType(), destVertices(nullptr), destIndices(nullptr) { };
	virtual ~VertexLoader() { };

	VertexType& getVertexType() { return vertexType; };
	virtual void setDestination(VertexSet& vertices, std::vector<uint16_t>& indices) { this->destVertices = &vertices; this->destIndices = &indices; };
	virtual void loadVertex() = 0;
};


class VertexFromUser : public VertexLoader
{
	size_t sourceVertexCount;
	const void* sourceVertices;
	std::vector<uint16_t>& sourceIndices;
	bool immediateMode;			/// If false, vertex data is loaded at loadVertex(). If true, it's loaded at setPointers().

	void loadData();

public:
	//VertexFromUser(size_t vertexCount, const void* vertexData, size_t indicesCount, const uint32_t* indices);
	VertexFromUser(const VertexType &vertexType, size_t vertexCount, const void* vertexData, std::vector<uint16_t>& indices, bool loadNow);
	~VertexFromUser() override;

	void setDestination(VertexSet& vertices, std::vector<uint16_t>& indices) override;
	void loadVertex() override;
};


class VertexFromFile : public VertexLoader
{
	const char* OBJfilePath;		//!< Path to model to load (set of vertex and indices)

public:
	VertexFromFile(const VertexType& vertexType, const char* modelPath);
	~VertexFromFile() override;

	/**
	*	@brief Populate the verticesand indices members with the vertex data from the mesh(OBJ file).
	*
	*	Fill the members "vertices" and "indices".
	*	An OBJ file consists of positions, normals, texture coordinatesand faces.Faces consist of an arbitrary amount of vertices, where each vertex refers to a position, normaland /or texture coordinate by index.
	*/
	void loadVertex() override;
};


#endif