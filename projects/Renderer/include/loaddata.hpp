#ifndef LOADDATA_HPP
#define LOADDATA_HPP

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "vertex.hpp"


/**
	@class VertexLoader
	@brief Abstract data type used for defining how to load Vertices and Indices (from file, from user creation, from variable...) into our ModelData object.

	The user must define a subclass where to override loadVertex() (pure virtual method). User can override setDestination() (virtual) too, and create new methods and variables.
	setDestination() is called by ModelData's constructor. loadVertex() is called by Renderer::fullConstructor() (loading thread).
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
	virtual void setDestination(VertexSet& vertices, std::vector<uint16_t>& indices) { this->destVertices = &vertices; this->destIndices = &indices; };	//!< Specify where vertex and index data is copied to (ModelData::vertices, ModelData::indices).
	virtual void loadVertex() = 0;			//!< Fill the structures pointed by destVertices and destIndices with data.
};


/// The user provides the already computed vertex and index data directly in-code. By default, data is copied directly, which lets the user delete the data source to save memory
class VertexFromUser_computed : public VertexLoader
{
	size_t sourceVertexCount;
	const void* sourceVertices;
	std::vector<uint16_t>& sourceIndices;
	bool immediateMode;			//!< If false, data is loaded at loadVertex(). If true, it's loaded at setDestination().

	void loadData();

public:
	//VertexFromUser(size_t vertexCount, const void* vertexData, size_t indicesCount, const uint32_t* indices);
	VertexFromUser_computed(const VertexType &vertexType, size_t vertexCount, const void* vertexData, std::vector<uint16_t>& indices, bool loadNow = true);
	~VertexFromUser_computed() override;

	void setDestination(VertexSet& vertices, std::vector<uint16_t>& indices) override;
	void loadVertex() override;
};


/// The user cannot provide the vertex and index data yet, but that will be computed in the worker.
class VertexFromUser_notComputed : public VertexLoader
{
public:
	VertexFromUser_notComputed(const char* modelPath) { };
	~VertexFromUser_notComputed() override { };

	void loadVertex() override { };
};


/**
	Get vertex and index data from a wavefront obj file using tiny_obj_loader.h. Vertex structure has to be specified.
	
	An OBJ file consists of vertex data (positions, normals, texture coordinates) and faces (indices). 
	Faces consist of an arbitrary amount of vertices and are defined by indices
	Each vertex refers to a position, normal and /or texture coordinate by index.
*/
class VertexFromFile : public VertexLoader
{
	const char* filePath;			//!< Path to model file to load (set of vertex and indices)

public:
	VertexFromFile(const char* modelPath);
	~VertexFromFile() override;

	void loadVertex() override;		//!< Populate the vertices and indices members with the vertex data from the mesh (OBJ file). An OBJ file consists of positions, normals, texture coordinatesand faces.Faces consist of an arbitrary amount of vertices, where each vertex refers to a position, normaland /or texture coordinate by index.
};


/**
	Get vertex and index data, and textures, from almost any file, using Assimp.hpp.

	Assimp: A file has a scene, which has tree. Each node has many meshes. Each mesh has many vertices (vertex data), many faces (each face has some indices), and one texture per material (NOT SURE ABOUT THIS) (diffuse, specular...). 
*/
class VertexFromFile2 : public VertexLoader
{
	std::string filePath;			//!< Path to model file to load (set of vertex and indices)
	std::vector<aiMesh*> meshes;	//!< Not used

	void processNode(aiNode* node, const aiScene* scene);	//!< Recursive function. It goes through each node getting all the meshes in each one.
	void processMesh(aiMesh* mesh, const aiScene* scene);
	//std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

public:
	VertexFromFile2(const char* modelPath);
	~VertexFromFile2() override;

	void loadVertex() override;
};


#endif