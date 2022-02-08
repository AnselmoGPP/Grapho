#ifndef LOADDATA_HPP
#define LOADDATA_HPP
// delete dataFromFile & modelPath variables in modelData, and method loadModel
// Change position of member variables

#include "vertex.hpp"
#include "environment.hpp"

class VertexLoader
{
public:
	VertexLoader() { };
	virtual ~VertexLoader() { };

	virtual void loadVertex(VertexSet& vertices, std::vector<uint32_t>& indices) = 0;
};


class VertexFromUser : public VertexLoader
{
public:
	VertexFromUser() { };
	~VertexFromUser() override { };

	void loadVertex(VertexSet& vertices, std::vector<uint32_t>& indices) override { };
};


class VertexFromFile : public VertexLoader
{
	const char* OBJfile;

public:
	VertexFromFile(const char* OBJfile);
	~VertexFromFile() override;

	/**
	*	@brief Populate the verticesand indices members with the vertex data from the mesh(OBJ file).
	*
	*	Fill the members "vertices" and "indices".
	*	An OBJ file consists of positions, normals, texture coordinatesand faces.Faces consist of an arbitrary amount of vertices, where each vertex refers to a position, normaland /or texture coordinate by index.
	*/
	void loadVertex(VertexSet& vertices, std::vector<uint32_t>& indices) override;
};


#endif