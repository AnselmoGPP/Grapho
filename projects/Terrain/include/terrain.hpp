#ifndef TERRAIN_HPP
#define TERRAIN_HPP

#include <iostream>
#include <cmath>
#include <map>
#include <list>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "renderer.hpp"
#include "toolkit.hpp"

#include "noise.hpp"
#include "common.hpp"

// -------------------------------

/// Generic Binary Search Tree Node.
template<typename T>
class QuadNode
{
public:
	QuadNode() { };
	QuadNode(const T& element, QuadNode* a = nullptr, QuadNode* b = nullptr, QuadNode* c = nullptr, QuadNode* d = nullptr) : element(element), a(a), b(b), c(c), d(d) { };
	~QuadNode() { if (a) delete a; if (b) delete b; if (c) delete c; if (d) delete d; };

	void setElement(const T& newElement) { element = newElement; }
	void setA(QuadNode<T>* node) { a = node; }
	void setB(QuadNode<T>* node) { b = node; }
	void setC(QuadNode<T>* node) { c = node; }
	void setD(QuadNode<T>* node) { d = node; }

	T& getElement() { return element; }
	QuadNode<T>* getA() { return a; }
	QuadNode<T>* getB() { return b; }
	QuadNode<T>* getC() { return c; }
	QuadNode<T>* getD() { return d; }

	bool isLeaf() { return !(a || b || c || d); }

private:
	// Ways to deal with keys and comparing records: (1) Key / value pairs (our choice), (2) Especial comparison method, (3) Passing in a comparator function.
	T element;
	QuadNode<T>* a, *b, *c, *d;
};

template<typename T, typename V>
void preorder(QuadNode<T>* root, V* visitor);

template<typename T, typename V>
void postorder(QuadNode<T>* root, V* visitor);

template<typename T, typename V>
void inorder(QuadNode<T>* root, V* visitor);


// Chunck systems -------------------------------

class Chunk
{
protected:
	Renderer& renderer;
	Noiser &noiseGen;
	glm::vec3 baseCenter;
	glm::vec3 groundCenter;

	float stride;
	unsigned numHorVertex, numVertVertex;
	float horBaseSize, vertBaseSize;		//!< Base surface from which computation starts
	float horChunkSize, vertChunkSize;		//!< Surface from where noise is applied

	std::vector<float> vertex;				//!< VBO[n][8] (vertex position[3], texture coordinates[2], normals[3])
	std::vector<uint16_t> indices;			//!< EBO[m][3] (indices[3])

	std::vector<Light*> lights;
	unsigned layer;					// Used in TerrainGrid for classifying chunks per layer

	size_t getPos(size_t x, size_t y) const { return y * numHorVertex + x; }
	glm::vec3 getVertex(size_t position) const { return glm::vec3(vertex[position * 8 + 0], vertex[position * 8 + 1], vertex[position * 8 + 2]); };
	glm::vec3 getNormal(size_t position) const { return glm::vec3(vertex[position * 8 + 5], vertex[position * 8 + 6], vertex[position * 8 + 7]); };
	virtual void computeSizes() = 0;		//!< Compute base size and chunk size

public:
	Chunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> center, float stride, unsigned numHorVertex, unsigned numVertVertex, std::vector<Light*> lights, unsigned layer);
	virtual ~Chunk();

	modelIterator model;			//!< Model iterator. It has to be created with render(), which calls app->newModel()
	bool modelOrdered;				//!< If true, the model creation has been ordered with app->newModel()

	virtual void computeTerrain(bool computeIndices, float textureFactor = 1.f) = 0;
	static void computeIndices(std::vector<uint16_t>& indices, unsigned numHorVertex, unsigned numVertVertex);		//!< Used for computing indices and saving them in a member or non-member buffer, which is passed by reference. 
	virtual void debug() { std::cout << "Nothing" << std::endl; };
	virtual void getSubBaseCenters(std::tuple<float, float, float>* centers) = 0;

	void render(const char* vertexShader, const char* fragmentShader, std::vector<texIterator>& usedTextures, std::vector<uint16_t>* indices);
	void updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj);

	unsigned getLayer() { return layer; }
	glm::vec3 getGroundCenter() { return groundCenter; }
	unsigned getNumVertex() { return numHorVertex * numVertVertex; }
	float getHorChunkSide() { return horChunkSize; };
	float getHorBaseSide() { return horBaseSize; };
};


/// Class used as the "element" of the QuadNode. Stores everything related to the object to render.
class PlainChunk : public Chunk
{
	void computeGridNormals();
	void computeSizes() override;

public:
	PlainChunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> center, float stride, unsigned numHorVertex, unsigned numVertVertex, std::vector<Light*> lights, unsigned layer = 0);
	~PlainChunk() { };

	/**
	*	TODO: edit comment
	*   @brief Compute VBO and EBO (creates some terrain specified by the user)
	*   @param noise Noise generator
	*   @param x0 Coordinate X of the first square's corner
	*   @param y0 Coordinate Y of the first square's corner
	*   @param stride Separation between vertex
	*   @param numVertex_X Number of vertex along the X axis
	*   @param numVertex_Y Number of vertex along the Y axis
	*   @param textureFactor How much of the texture surface will fit in a square of 4 contiguous vertex
	*/
	void computeTerrain(bool computeIndices, float textureFactor = 1.f) override;
	void getSubBaseCenters(std::tuple<float, float, float>* centers) override;
};


class SphericalChunk : public Chunk
{
	//glm::vec3 cubePlane;
	glm::vec3 nucleus;
	float radius;
	glm::vec3 xAxis, yAxis;		// Vectors representing the relative XY coord. system of the cube side plane.

	void computeGridNormals(glm::vec3 pos0, glm::vec3 xAxis, glm::vec3 yAxis);
	void computeSizes() override;

public:
	SphericalChunk::SphericalChunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, std::vector<Light*> lights, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned layer = 0);
	~SphericalChunk() { };

	void computeTerrain(bool computeIndices, float textureFactor = 1.f) override;
	void getSubBaseCenters(std::tuple<float, float, float>* centers) override;

	void debug() override
	{
		//std::cout << baseCenter.x << ", " << baseCenter.y << ", " << baseCenter.z << " / "
		//	<< groundCenter.x << ", " << groundCenter.y << ", " << groundCenter.z << " / "
		//	<< stride << " / " << numHorVertex << '/' << numVertVertex << " / " 
		//	<< horSize << ", " << vertSize << " / "
		//	<< nucleus.x << ", " << nucleus.y << ", " << nucleus.z << " / "
		//	<< cubePlane.x << ", " << cubePlane.y << ", " << cubePlane.z << " / "
		//	<< radius << std::endl;
	}
};


// Grid systems -------------------------------

class DynamicGrid
{
public:
	DynamicGrid(glm::vec3 camPos, std::vector<Light*> lights, Renderer& renderer, Noiser noiseGenerator, unsigned activeTree, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier);
	virtual ~DynamicGrid();

	glm::vec3 camPos;
	glm::mat4 view;
	glm::mat4 proj;

	void addTextures(const std::vector<texIterator>& textures);
	void addShaders(const char* fragShader, const char* vertShader);
	void updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj);
	void updateTree(glm::vec3 newCamPos);
	unsigned getTotalNodes() { return chunks.size(); }
	unsigned getloadedChunks() { return loadedChunks; }
	unsigned getRenderedChunks() { return renderedChunks; }

protected:
	QuadNode<Chunk*>* root[2];
	std::map<std::tuple<float, float, float>, Chunk*> chunks;
	std::vector<uint16_t> indices;
	std::vector<texIterator> textures;
	std::vector<Light*> lights;
	Renderer& renderer;
	Noiser noiseGenerator;
	std::string vertShader;
	std::string fragShader;

	unsigned activeTree;
	unsigned loadedChunks;
	unsigned renderedChunks;

	// Configuration data
	float rootCellSize;
	size_t numSideVertex;		// Number of vertex per square side
	size_t numLevels;			// Levels of resolution
	size_t minLevel;			// Minimum level used (actual levels used = numLevels - minLevel) (example: 7-3=4 -> 800,400,200,100)
	float distMultiplier;		// Relative distance (when distance camera-node's center is <relDist, the node is subdivided.

	void createTree(QuadNode<Chunk*>* node, size_t depth);			//!< Recursive
	bool fullConstChunks(QuadNode<Chunk*>* node);					//!< Recursive (Preorder traversal)
	void changeRenders(QuadNode<Chunk*>* node, bool renderMode);	//!< Recursive (Preorder traversal)
	void updateUBOs_help(QuadNode<Chunk*>* node);					//!< Recursive (Preorder traversal)
	void removeFarChunks(unsigned relDist, glm::vec3 camPosNow);	//!< Remove those chunks that are too far from camera

	virtual QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned layer) = 0;
	virtual std::tuple<float, float, float> closestCenter() = 0;	//!< Find closest center to the camera of the biggest chunk (i.e. lowest level chunk).
};


class TerrainGrid : public DynamicGrid
{
public:
	/**
	*	@brief Constructors
	*	@param noiseGenerator Used for generating noise
	*	@param rootCellSize Size of the entire scenario
	*	@param numSideVertex Number of vertex per side in each chunk
	*	@param numLevels Number of levels of resolution
	*	@param minLevel Minimum level rendered. Used for avoiding rendering too big chunks.
	*	@param distMultiplier Distance (relative to a chunk side size) at which the chunk is subdivided.
	*/
	TerrainGrid(Renderer& renderer, Noiser noiseGenerator, std::vector<Light*> lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier);

private:
	QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned layer) override;
	std::tuple<float, float, float> closestCenter() override;
};


class PlanetGrid : public DynamicGrid
{
public:
	PlanetGrid(Renderer& renderer, Noiser noiseGenerator, std::vector<Light*> lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter);

private:
	float radius;
	glm::vec3 nucleus;
	glm::vec3 cubePlane;
	glm::vec3 cubeSideCenter;

	QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned layer) override;
	std::tuple<float, float, float> closestCenter() override;
};


// Definitions ----------------------------------------------------------------

template<typename T, typename V>
void preorder(QuadNode<T>* root, V* visitor)
{
	if (!root) return;
	visitor(root);
	preorder(root.getLeft());
	preorder(root.getRight());
}
/*
template<typename T, typename V, typename A>
void preorder(QuadNode<T>* root, V* visitor, A &params)
{
	if (!root) return;
	visitor(root, params);
	preorder(root.getLeft());
	preorder(root.getRight());
}
*/
template<typename T, typename V>
void postorder(QuadNode<T>* root, V* visitor)
{
	if (!root) return;
	inorder(root->getLeft());
	inorder(root->getRight());
	visitor(root);
}

template<typename T, typename V>
void inorder(QuadNode<T>* root, V* visitor)
{
	if (!root) return;
	inorder(root->getLeft());
	visitor(root);
	inorder(root->getRight());
}

//void updateUBOs_visitor(QuadNode<Chunk*>* node, const TerrainGrid &terrGrid);


#endif