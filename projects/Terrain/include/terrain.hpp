#ifndef TERRAIN_HPP
#define TERRAIN_HPP

#include <iostream>
#include <cmath>
#include <map>
#include <list>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "noise.hpp"

// Given camPos, compute maximum terrain margins (array)
// Given that, compute margins for each node, and their center

/// Generic Binary Search Tree Node.
template<typename T>
class QuadNode
{
public:
	QuadNode() { };
	QuadNode(const T& element, QuadNode* a = nullptr, QuadNode* b = nullptr, QuadNode* c = nullptr, QuadNode* d = nullptr);
	~QuadNode();

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


/// Class used as the "element" of the QuadNode. Stores everything related to the object to render.
class Chunk
{
	glm::vec3 center;
	float sideXSize;
	unsigned numVertexX, numVertexY;

	void computeGridNormals(float stride, noiseSet& noise);
	size_t getPos(size_t x, size_t y) const;
	glm::vec3 getVertex(size_t position) const;

public:
	Chunk(glm::vec3 center, float sideXSize, unsigned numVertexX, unsigned numVertexY);
	Chunk(std::tuple<float, float, float> center, float sideXSize, unsigned numVertexX, unsigned numVertexY);
	~Chunk() { };

	//float(*vertex)[8];			///< VBO (vertex position[3], texture coordinates[2], normals[3])
	std::vector<float> vertex;		///< VBO[n][8] (vertex position[3], texture coordinates[2], normals[3])
	std::vector<uint32_t> indices;	///< EBO[m][3] (indices[3])

	void reset(glm::vec3 center, float sideXSize, unsigned numVertexX, unsigned numVertexY);
	void reset(std::tuple<float, float, float> center, float sideXSize, unsigned numVertexX, unsigned numVertexY);

	/**
	*   @brief Compute VBO and EBO (creates some terrain specified by the user)
	*   @param noise Noise generator
	*   @param x0 Coordinate X of the first square's corner
	*   @param y0 Coordinate Y of the first square's corner
	*   @param stride Separation between vertex
	*   @param numVertex_X Number of vertex along the X axis
	*   @param numVertex_Y Number of vertex along the Y axis
	*   @param textureFactor How much of the texture surface will fit in a square of 4 contiguous vertex
	*/
	void computeTerrain(noiseSet& noise, float textureFactor = 1.f);

	unsigned getNumVertex() { return numVertexX * numVertexY; }
	glm::vec3 getCenter() { return center; }
	float getSide() { return sideXSize; }
};


// In map chunks, is it possible that two chunks with different depth have same center?
// Use int instead of floats for map key?
class TerrainGrid
{
	QuadNode<Chunk*>* root;
	std::map<std::tuple<float, float, float>, Chunk*> chunks;
	std::list<QuadNode<Chunk>> recicledNodes;	// <<<
	std::vector<std::vector<uint32_t>> indices;	// <<<

	noiseSet noiseGenerator;
	glm::vec3 camPos;
	size_t nodeCount;

	// Configuration data
	float rootCellSize;
	size_t numSideVertex;		// Number of vertex per square side
	size_t numLevels;			// Levels of resolution
	size_t minLevel;			// Minimum level used (actual levels used = numLevels - minLevel) (example: 7-3=4 -> 800,400,200,100)
	float distMultiplier;		// Relative distance (when distance camera-node's center is <relDist, the node is subdivided.

	std::tuple<float, float, float> closestCenter();
	void update(QuadNode<Chunk*> *node, size_t depth);

	//void insert(const Chunk& element);

	//BSTNode<T>* findHelp(BSTNode<T>* node, K key);
	//BSTNode<T>* insertHelp(BSTNode<T>* node, const K& key, const E& element);
	//BSTNode<T>* removeHelp(BSTNode<T>* node, const K& key);
	//void printHelp(BSTNode<T>* node);
	//BSTNode<T>* deleteMin(BSTNode<T>* node);

	//BSTNode<T>* getMin(BSTNode<T>* node);

public:
	TerrainGrid(noiseSet noiseGenerator, glm::vec3 camPos, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier);
	~TerrainGrid();

	void updateTree(glm::vec3 newCamPos);

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

template<typename T>
QuadNode<T>::QuadNode(const T& element, QuadNode* a, QuadNode* b, QuadNode* c, QuadNode* d) 
	: element(element), a(a), b(b), c(c), d(d) { }

template<typename T>
QuadNode<T>::~QuadNode() 
{ 
	if (a) delete a; 
	if (b) delete b; 
	if (c) delete c; 
	if (d) delete d; 
};


//-----------------------------------------------------------------------------

/// Bidimensional index (x, y) that satisfies the "Compare" set of requirements for its use in std::map.
class BinaryKey
{
public:
    int x;
    int y;

    /// Constructor
    BinaryKey(int first, int second);

    /// Set (x, y) values simultaneously
    void set(int first, int second);

    /// Comparison (binary predicate). Strict weak ordering (true if a precedes b)
    bool operator <( const BinaryKey &rhs ) const;

    /// Equivalence. True if a == b, false otherwise.
    bool operator ==( const BinaryKey &rhs ) const;
};

/*
 * TODO:
 * Circular area of chunks
 * Update only new chunks. Move or delete the others
 */
class terrainChunks
{
    //glm::vec3 viewerPosition;
    //std::vector<std::vector<terrainChunk>> chunk;
    //std::vector<std::vector<glm::vec2>> chunkName;

public:
    noiseSet noise;             ///< Noise generator
    float    maxViewDist;       ///< Maximum view distance from viewer
    float    chunkSize;         ///< Size of each chunk (meters)
    int      chunksVisible;     ///< Number of chunkSizes for reaching maxViewDist
    int      vertexPerSide;     ///< Number of vertex per chunk's side

    std::map<BinaryKey, NoiseSurface> chunkDict;    ///< Collection of all the chunks (as a dictionary)

    terrainChunks(noiseSet noise, float maxViewDist, float chunkSize, unsigned vertexPerSide);
    ~terrainChunks();

    int getNumVertex();
    int getNumIndices();
    int getMaxViewDist();

    void updateVisibleChunks(glm::vec3 viewerPos);
    void updateTerrainParameters(noiseSet noise, float maxViewDist, float chunkSize, unsigned vertexPerSide);
    void setNoise(noiseSet newNoise);
};

#endif
