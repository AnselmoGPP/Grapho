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

/*
	QuadNode

	Chunk
		PlainChunk
		PlanetChunk
			SphereChunk	

	DynamicGrid (QuadNode)
		TerrainGrid (PlainChunk)
		PlanetGrid (SphericalChunk)
			SphereGrid

	Planet (PlanetGrid)
		Sphere
*/

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


// Chunk -------------------------------

enum side{ right, left, up, down };

/**
	Class used as the "element" of the QuadNode. Stores everything related to the object to render.
	Process followed by DynamicGrid:
	  1. computeIndices()
	  2. getSubBaseCenters()
	  3. Constructor()
	  4. computeTerrain()
	  5. render()
	  6. updateUBOs()
*/
class Chunk
{
protected:
	Renderer& renderer;
	glm::vec3 baseCenter;
	glm::vec3 groundCenter;

	float stride;
	unsigned numHorVertex, numVertVertex;
	float horBaseSize, vertBaseSize;		//!< Base surface from which computation starts (plane)
	float horChunkSize, vertChunkSize;		//!< Surface from where noise is applied (sphere section)
	int numAttribs;							//!< Number of attributes per vertex (9)

	std::vector<float> vertex;				//!< VBO[n][6] (vertex position[3], normals[3])
	std::vector<uint16_t> indices;			//!< EBO[m][3] (indices[3])

	glm::vec3 getVertex(size_t position) const { return glm::vec3(vertex[position * numAttribs + 0], vertex[position * numAttribs + 1], vertex[position * numAttribs + 2]); };
	glm::vec3 getNormal(size_t position) const { return glm::vec3(vertex[position * numAttribs + 3], vertex[position * numAttribs + 4], vertex[position * numAttribs + 5]); };
	virtual void computeSizes() = 0;		//!< Compute base size and chunk size

public:
	Chunk(Renderer& renderer, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned depth, unsigned chunkID);
	virtual ~Chunk();

	modelIterator model;			//!< Model iterator. It has to be created with render(), which calls app->newModel()
	bool modelOrdered;				//!< If true, the model creation has been ordered with app->newModel()

	const unsigned depth;			//!< Range: [0, x]. Depth of this chunk. Useful in a grid of chunks with different lod.
	glm::vec4 sideDepths;			//!< Range: [0, x]. Depth of neighbouring chunks (right, left, up, down). Useful for adjusting chunk borders to neighbors' chunks.
	unsigned chunkID;				//!< Range: [1, x]. Unique number per chunk per depth. Useful for computing sideDepths.

	virtual void computeTerrain(bool computeIndices) = 0;
	static void computeIndices(std::vector<uint16_t>& indices, unsigned numHorVertex, unsigned numVertVertex);		//!< Used for computing indices and saving them in a member or non-member buffer, which is passed by reference. 
	virtual void getSubBaseCenters(std::tuple<float, float, float>* centers) = 0;

	void render(shaderIter vertexShader, shaderIter fragmentShader, std::vector<texIterator>& usedTextures, std::vector<uint16_t>* indices, unsigned numLights, bool transparency);
	void updateUBOs(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos, LightSet& lights, float time, glm::vec3 planetCenter = glm::vec3(0,0,0));

	void setSideDepths(unsigned a, unsigned b, unsigned c, unsigned d);
	glm::vec3 getGroundCenter()	{ return groundCenter; }
	unsigned getNumVertex()		{ return numHorVertex * numVertVertex; }
	float getHorChunkSide()		{ return horChunkSize; };
	float getHorBaseSide()		{ return horBaseSize; };
};


class PlainChunk : public Chunk
{
	Noiser* noiseGen;

	void computeGridNormals();
	void computeSizes() override;

public:
	PlainChunk(Renderer& renderer, Noiser* noiseGenerator, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned depth = 0, unsigned chunkID = 0);
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
	void computeTerrain(bool computeIndices) override;
	void getSubBaseCenters(std::tuple<float, float, float>* centers) override;
};


class PlanetChunk : public Chunk
{
protected:
	Noiser* noiseGen;
	glm::vec3 nucleus;
	float radius;
	glm::vec3 xAxis, yAxis;		// Vectors representing the relative XY coordinate system of the cube side plane.

	void computeGridNormals(glm::vec3 pos0, glm::vec3 xAxis, glm::vec3 yAxis, unsigned numHorV, unsigned numVerV);
	void computeGapFixes();
	void computeSizes() override;

public:
	PlanetChunk::PlanetChunk(Renderer& renderer, Noiser* noiseGenerator, glm::vec3 cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned depth = 0, unsigned chunkID = 0);
	virtual ~PlanetChunk() { };

	virtual void computeTerrain(bool computeIndices) override;
	void getSubBaseCenters(std::tuple<float, float, float>* centers) override;
};


/// Plain sphere chunk without noise
class SphereChunk : public PlanetChunk
{
public:
	SphereChunk(Renderer& renderer, glm::vec3 cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned depth = 0, unsigned chunkID = 0);

	void computeTerrain(bool computeIndices) override;
};


// Grid systems -------------------------------

/**
	Creates a set of Chunk objects that make up a terrain. These Chunks are replaced with other Chunks in order to present 
	higher resolution Chunks near the camera. To make chunks' vertices fit other chunks of different depth (up to n 
	depths), then number of side vertices must be = X·2^n + 1
	Process:
		1. Constructor()
			- Chunk::computeIndices()
		2. addTextures() (once)
		3. addShaders() (once)
		4. updateTree() (each frame)
			- Chunk::getSubBaseCenters()
			- Chunk::constructor()
			- Chunk::computeTerrain()
			- Chunk::render()
		5. updateUBOs() (each frame)
			- Chunk::updateUBOs()
*/
class DynamicGrid
{
public:
	DynamicGrid(glm::vec3 camPos, LightSet& lights, Renderer* renderer, unsigned activeTree, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, bool transparency);
	virtual ~DynamicGrid();

	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 camPos;
	LightSet* lights;
	float time;

	void addTextures(const std::vector<texIterator>& textures);				//!< Add textures ids of already loaded textures.
	void addShaders(shaderIter vertexShader, shaderIter fragmentShader);	//!< Add shaders ids of already loaded shaders.
	void updateTree(glm::vec3 newCamPos);
	void updateUBOs(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos, LightSet& lights, float time);
	unsigned getTotalNodes() { return chunks.size(); }
	unsigned getloadedChunks() { return loadedChunks; }
	unsigned getRenderedChunks() { return renderedChunks; }
	void toLastDraw() { putToLastDraw(root[activeTree]); };					//!< Call it after updateTree(), so the correct tree is put last to draw
	
protected:
	QuadNode<Chunk*>* root[2];
	std::map<std::tuple<float, float, float>, Chunk*> chunks;
	std::vector<uint16_t> indices;
	std::vector<texIterator> textures;
	//std::vector<Light*> lights;
	Renderer* renderer;
	shaderIter vertShader;
	shaderIter fragShader;

	unsigned activeTree;
	unsigned loadedChunks;
	unsigned renderedChunks;

	// Configuration data
	float rootCellSize;
	size_t numSideVertex;		// Number of vertex per square side
	size_t numLevels;			// Number of LOD
	size_t minLevel;			// Minimum level used(from 0 to numLevels-1) (actual levels used = numLevels - minLevel) (example: 7-3=4 -> 800,400,200,100)
	float distMultiplier;		// Relative distance (when distance camera-node's center is <relDist, the node is subdivided.
	bool transparency;

	void createTree(QuadNode<Chunk*>* node, size_t depth);			//!< Recursive
	bool fullConstChunks(QuadNode<Chunk*>* node);					//!< Recursive (Preorder traversal)
	void changeRenders(QuadNode<Chunk*>* node, bool renderMode);	//!< Recursive (Preorder traversal)
	void updateUBOs_help(QuadNode<Chunk*>* node);					//!< Recursive (Preorder traversal)
	void putToLastDraw(QuadNode<Chunk*>* node);						//!< Recursive (Preorder traversal)
	void updateChunksSideDepths(QuadNode<Chunk*>* node);			//!< Breath-first search for computing the depth that each side of the chunk must fit.
	void updateChunksSideDepths_help(std::list<QuadNode<Chunk*>*> &queue, QuadNode<Chunk*>* currentNode); //!< Helper method. Computes the depth of each side of a chunk based on adjacent chunks (right and down).
	void restartSideDepths(QuadNode<Chunk*>* node);					//!< Set side depths of all nodes in the tree to 0.
	void removeFarChunks(unsigned relDist, glm::vec3 camPosNow);	//!< Remove those chunks that are too far from camera
	glm::vec4 getChunkIDs(unsigned parentID, unsigned depth);

	virtual QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID) = 0;
	virtual std::tuple<float, float, float> closestCenter() = 0;	//!< Find closest center to the camera of the biggest chunk (i.e. lowest level chunk).
};


class TerrainGrid : public DynamicGrid
{
public:
	/**
	*	@brief Constructor
	*	@param noiseGenerator Used for generating noise
	*	@param rootCellSize Size of the entire scenario
	*	@param numSideVertex Number of vertex per side in each chunk
	*	@param numLevels Number of levels of resolution
	*	@param minLevel Minimum level rendered. Used for avoiding rendering too big chunks.
	*	@param distMultiplier Distance (relative to a chunk side size) at which the chunk is subdivided.
	*/
	TerrainGrid(Renderer* renderer, Noiser* noiseGenerator, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, bool transparency);

private:
	Noiser* noiseGen;
	QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID) override;
	std::tuple<float, float, float> closestCenter() override;
};


class PlanetGrid : public DynamicGrid
{
public:
	PlanetGrid(Renderer* renderer, Noiser* noiseGenerator, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter, bool transparency);
	virtual ~PlanetGrid() { };

	float getRadius();

protected:
	Noiser* noiseGen;
	float radius;
	glm::vec3 nucleus;
	glm::vec3 cubePlane;
	glm::vec3 cubeSideCenter;

	virtual QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID) override;
	std::tuple<float, float, float> closestCenter() override;
};


class SphereGrid : public PlanetGrid
{
public:
	SphereGrid(Renderer* renderer, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter, bool transparency);
	
	QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID) override;
};


// Planet ----------------------------------------------------------------

//enum dir{ pX, nX, pY, nY, pZ, nZ };

/**
	Six PlanetGrid objects that make up a planet and update the internal SphericalChunk objects depending upon camera position.
	The number of side vertices should be odd if you want to make them fit with an equivalent chunk but twice its size.
	1. Constructor
		- DynamicGrid.constructor()
	2. addResources() (once)
		- DynamicGrid.addTextures()
		- DynamicGrid.addShaders()
	3. updateState() (each frame)
		- DynamicGrid.updateTree()
		- DynamicGrid.updateUBOs()
*/
struct Planet
{
	Planet(Renderer* renderer, Noiser* noiseGenerator, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency);
	virtual ~Planet();

	void addResources(const std::vector<texIterator>& textures, shaderIter vertexShader, shaderIter fragmentShader);			//!< Add textures and shader
	void updateState(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj, LightSet& lights, float frameTime);	//!< Update tree and UBOs
	void toLastDraw();
	float getSphereArea();																										//!< Given planet radius, get sphere's area

	const float radius;
	const glm::vec3 nucleus;

protected:
	Noiser* noiseGen;
	PlanetGrid* planetGrid_pZ;
	PlanetGrid* planetGrid_nZ;
	PlanetGrid* planetGrid_pY;
	PlanetGrid* planetGrid_nY;
	PlanetGrid* planetGrid_pX;
	PlanetGrid* planetGrid_nX;

	bool readyForUpdate;

	virtual float callBack_getFloorHeight(const glm::vec3& pos);	//!< Callback example
};


struct Sphere : public Planet
{
	float callBack_getFloorHeight(const glm::vec3& pos) override;	//!< Callback example

public:
	Sphere(Renderer* renderer, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency);
	~Sphere();
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
