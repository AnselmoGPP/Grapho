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

class Chunk
{
protected:
	Renderer& renderer;
	Noiser &noiseGen;
	glm::vec3 baseCenter;
	glm::vec3 groundCenter;

	float horSize;
	unsigned numHorVertex, numVertVertex;

	std::vector<float> vertex;		//!< VBO[n][8] (vertex position[3], texture coordinates[2], normals[3])
	std::vector<uint16_t> indices;	//!< EBO[m][3] (indices[3])

	unsigned layer;					// Used in TerrainGrid for classifying chunks per layer

	virtual void computeGridNormals(float stride) = 0;

	size_t getPos(size_t x, size_t y) const { return y * numHorVertex + x; }
	glm::vec3 getVertex(size_t position) const { return glm::vec3(vertex[position * 8 + 0], vertex[position * 8 + 1], vertex[position * 8 + 2]); };

public:
	Chunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> center, float horSize, unsigned numHorVertex, unsigned numVertVertex, unsigned layer);
	virtual ~Chunk();

	modelIterator model;			//!< Model iterator. It has to be created with render(), which calls app->newModel()

	bool modelOrdered;				//!< If true, the model creation has been ordered with app->newModel()

	virtual void computeTerrain(bool computeIndices, float textureFactor) = 0;
	static void computeIndices(std::vector<uint16_t>& indices, unsigned numHorVertex, unsigned numVertVertex);		//!< Used for computing indices and saving them in a member or non-member buffer, which is passed by reference. 

	void render(const char* vertexShader, const char* fragmentShader, std::vector<texIterator>& usedTextures, std::vector<uint16_t>* indices);
	void updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj);

	unsigned getLayer() { return layer; }
	glm::vec3 getCenter() { return groundCenter; }
	unsigned getNumVertex() { return numHorVertex * numVertVertex; }
	float getSide() { return horSize; }
};

/// Class used as the "element" of the QuadNode. Stores everything related to the object to render.
class PlainChunk : public Chunk
{
	void computeGridNormals(float stride) override;

public:
	PlainChunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> center, float horSize, unsigned numHorVertex, unsigned numVertVertex, unsigned layer = 0);
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
	void computeTerrain(bool computeIndices, float textureFactor) override;
};


enum CubeSide { posX, posY, posZ, negX, negY, negZ };

/*
	TODO:
		BUG: Elements not already destroyed when calling cleanup()
		Sphere camera (set nucleus) and encapsulate parameters correctly
		Biplanar texture (shader) improve (and make normals/tangents correctly)
		Fix textures (normals) in cube sides borders
		Light object is initialized in each chunck
*/
class SphericalChunk : public Chunk
{
	CubeSide cubePlane;
	glm::vec3 nucleus;
	float radius;

	void computeGridNormals(float stride) override;

public:
	SphericalChunk::SphericalChunk(Renderer& renderer, Noiser& noiseGen, std::tuple<float, float, float> sqrSideCenter, float horSize, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, CubeSide cubePlane, unsigned layer = 0);
	~SphericalChunk() { };

	void computeTerrain(bool computeIndices, float textureFactor) override;
};


/*
	x Aligerar Chunk
	x Enderezar textura terreno
	x Modify texture multiplier (for now, it's useful for debugging)
	x Chunk corners shadows
	x New noiser
	x (Non-possible) Don't create chunks of non-leaf nodes
	x (better not) rend to app
	x node-getElement() to chunk*
	x Encapsulate chunk/node creation
	fix gaps when loading chunks (un-render chunks after their replacements have been rendered)
	>>> Remove/Recicle out-of-range chunks
	exception when going to far -> try your own octaves implementation
	Destructor TerrainGrid/Chunks that correctly deletes chunks
	x Follow camera
	Recicle chunks
	Remove some non-visible chunks when there are too much of them (saves memory)
	computeTerrain may cause bottleneck (no second thread loading)
*/
class TerrainGrid
{
	QuadNode<PlainChunk*>* root[2];
	std::map<std::tuple<float, float, float>, PlainChunk*> chunks;
	std::list<QuadNode<PlainChunk>> recicledNodes;	// <<<
	std::vector<uint16_t> indices;
	std::vector<texIterator> textures;

	unsigned activeTree;
	Renderer &renderer;
	Noiser noiseGenerator;

	// Configuration data
	float rootCellSize;
	size_t numSideVertex;		// Number of vertex per square side
	size_t numLevels;			// Levels of resolution
	size_t minLevel;			// Minimum level used (actual levels used = numLevels - minLevel) (example: 7-3=4 -> 800,400,200,100)
	float distMultiplier;		// Relative distance (when distance camera-node's center is <relDist, the node is subdivided.

	std::tuple<float, float, float> closestCenter();

	void createTree(QuadNode<PlainChunk*>* node, size_t depth);			//!< Recursive
	void updateUBOs_help(QuadNode<PlainChunk*>* node);					//!< Recursive (Preorder traversal)
	bool fullConstChunks(QuadNode<PlainChunk*>* node);					//!< Recursive (Preorder traversal)
	void changeRenders(QuadNode<PlainChunk*>* node, bool renderMode);	//!< Recursive (Preorder traversal)
	QuadNode<PlainChunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned layer);
	void removeFarChunks(unsigned relDist, glm::vec3 camPosNow);

	//void insert(const Chunk& element);

	//BSTNode<T>* findHelp(BSTNode<T>* node, K key);
	//BSTNode<T>* insertHelp(BSTNode<T>* node, const K& key, const E& element);
	//BSTNode<T>* removeHelp(BSTNode<T>* node, const K& key);
	//void printHelp(BSTNode<T>* node);
	//BSTNode<T>* deleteMin(BSTNode<T>* node);

	//BSTNode<T>* getMin(BSTNode<T>* node);

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
	TerrainGrid(Renderer &renderer, Noiser noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier);
	~TerrainGrid();

//	void addApp(Renderer& app);
	void addTextures(const std::vector<texIterator>& textures);

	void updateTree(glm::vec3 newCamPos);
	void updateUBOs(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj);

	glm::vec3 camPos;
	glm::mat4 view;
	glm::mat4 proj;

	size_t nodeCount;
	size_t leafCount;
	unsigned getTotalNodes() { return chunks.size(); }

	//void remove(const K& key);
	//T& find(const K& key) { return (findHelp(root, key))->getElement(); }

	//void clear();
	//void removeRoot();
	//size_t size() { return nodeCount; }
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

void updateUBOs_visitor(QuadNode<Chunk*>* node, const TerrainGrid &terrGrid);


#endif
