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
	//QuadNode() { };
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

	bool isLeaf() { return !(a || b || c || d); }	//!< Is leaf if all subnodes are null; otherwise, it's not. Full binary tree: Every node either has zero children [leaf node] or two children. All leaf nodes have an element associated. There are no nodes with only one child. Each internal node has exactly two children.
	//bool isLeaf_BST() { return (a); }	//!< For simple Binary Trees (BT).

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
	glm::vec3 geoideCenter;
	glm::vec3 groundCenter;
	glm::vec3 center;			//!< Alternative center defined by 

	float stride;
	unsigned numHorVertex, numVertVertex;
	float horBaseSize, vertBaseSize;		//!< Base surface from which computation starts (plane)
	float horChunkSize, vertChunkSize;		//!< Surface where noise is applied (sphere section)
	int numAttribs;							//!< Number of attributes per vertex (9)

	VerticesLoader* vertexData;
	std::vector<float> vertex;				//!< VBO[n][9] (vertex position[3], normals[3], gap-fix data[3])
	std::vector<uint16_t> indices;			//!< EBO[m][3] (indices[3])

	glm::vec3 getVertex(size_t position) const;
	glm::vec3 getNormal(size_t position) const;
	virtual void computeSizes() = 0;		//!< Compute base size and chunk size

public:
	Chunk(Renderer& renderer, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned depth, unsigned chunkID);
	virtual ~Chunk();

	modelIter model;				//!< Model iterator. It has to be created with render(), which calls app->newModel()
	bool modelOrdered;				//!< If true, the model creation has been ordered with app->newModel()
	bool isVisible;					//!< Used during tree construction and loading for not rendering non-visible chunks in DynamicGrid.

	const unsigned depth;			//!< Range: [0, n]. Depth of this chunk. Each depth represent a lod (for a grid of chunks with different lod).
	glm::vec4 sideDepths;			//!< Range: [0, x]. Depth of neighbouring chunks (right, left, up, down). Useful for adjusting chunk borders to neighboring chunks.
	unsigned chunkID;				//!< Range: [1, x]. Unique number per chunk per depth. Useful for computing sideDepths.

	virtual void computeTerrain(bool computeIndices) = 0;
	static void computeIndices(std::vector<uint16_t>& indices, unsigned numHorVertex, unsigned numVertVertex);		//!< Used for computing indices and saving them in a member or non-member buffer, which is passed by reference. 
	virtual void getSubBaseCenters(std::tuple<float, float, float>* centers) = 0;
	virtual glm::vec3 getCenter();
	void deleteModel();

	void render(std::vector<ShaderLoader>& shaders, std::vector<TextureLoader>& textures, std::vector<uint16_t>* indices, unsigned numLights, bool transparency);
	void updateUBOs(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos, const LightSet& lights, float time, float camHeight, glm::vec3 planetCenter = glm::vec3(0,0,0));

	void setSideDepths(unsigned a, unsigned b, unsigned c, unsigned d);
	glm::vec3 getGeoideCenter() { return geoideCenter; }
	glm::vec3 getGroundCenter() { return groundCenter; }
	unsigned getNumVertex()		{ return numHorVertex * numVertVertex; }
	float getHorChunkSide()		{ return horChunkSize; };
	float getHorBaseSide()		{ return horBaseSize; };
	std::vector<float>* getVertices() { return &vertex; }
};

/// Plain chunk with noise
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

/// Sphere chunk with noise
class PlanetChunk : public Chunk
{
protected:
	std::shared_ptr<Noiser> noiseGen;
	glm::vec3 nucleus;
	float radius;
	glm::vec3 xAxis, yAxis;			//!< Vectors representing the relative XY coordinate system of the cube side plane.

	void computeGridNormals(glm::vec3 pos0, glm::vec3 xAxis, glm::vec3 yAxis, unsigned numHorV, unsigned numVerV);
	void computeGapFixes();
	void computeSizes() override;

public:
	PlanetChunk::PlanetChunk(Renderer& renderer, std::shared_ptr<Noiser> noiseGenerator, glm::vec3 cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned depth = 0, unsigned chunkID = 0);
	virtual ~PlanetChunk() { };

	virtual void computeTerrain(bool computeIndices) override;
	void getSubBaseCenters(std::tuple<float, float, float>* centers) override;
	float getRadius();
};


/// Sphere chunk without noise
class SphereChunk : public PlanetChunk
{
public:
	SphereChunk(Renderer& renderer, glm::vec3 cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned depth = 0, unsigned chunkID = 0);

	void computeTerrain(bool computeIndices) override;
};


// Grid systems -------------------------------

/**
	Creates a set of Chunk objects that make up a terrain. These Chunks are replaced with other Chunks in order to present 
	higher resolution. The number of side vertices should be odd if you want to make them fit with an equivalent chunk but 
	twice its size. Chunks near the camera. To make chunks' vertices fit other chunks of different depth (up to n depths), 
	then number of side vertices must be = X·2^n + 1 (Examples for n=2: 21, 25, 29, 33, 37)
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
	DynamicGrid(glm::vec3 camPos, Renderer* renderer, unsigned activeTree, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, bool transparency);
	virtual ~DynamicGrid();

	//unsigned char* ubo;
	glm::vec3 camPos;
	unsigned numLights;

	void addResources(const std::vector<ShaderLoader>& shadersInfo, const std::vector<TextureLoader>& texturesInfo);		//!< Add textures and shaders info
	void updateTree(glm::vec3 newCamPos, unsigned numLights);
	void updateUBOs(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos, const LightSet& lights, float time, float groundHeight);
	void toLastDraw();														//!< Call it after updateTree(), so the correct tree is put last to draw
	void getActiveLeafChunks(std::vector<Chunk*>& dest, unsigned depth);	//!< Get active chunks with depth >= X in the active tree 
	
	// Testing
	unsigned numChunks();				//!< Number of chunks (loaded and not loaded)
	unsigned numChunksOrdered();		//!< Number of ordered chunks (those fully constructed or pending to be so)
	unsigned numActiveLeafChunks();		//!< Number of leaf chunks in the active tree

protected:
	QuadNode<Chunk*>* root[2];										//!< Active and non-active tree
	std::map<std::tuple<float, float, float>, Chunk*> chunks;		//!< All chunks
	std::vector<Chunk*> visibleLeafChunks[2];						//!< Visible leaf chunks of each tree
	Renderer* renderer;
	std::vector<uint16_t> indices;
	std::vector<ShaderLoader> shaders;
	std::vector<TextureLoader> textures;
	unsigned activeTree, nonActiveTree;

	// Configuration data
	float rootCellSize;
	size_t numSideVertex;		//!< Number of vertices per square side. More information in class description.
	size_t numLevels;			//!< Number of LOD
	size_t minLevel;			//!< Minimum level used(from 0 to numLevels-1) (actual levels used = numLevels - minLevel) (example: 7-3=4 -> 800,400,200,100)
	float distMultiplier;		//!< Relative distance (when distance camera-node's center is < relDist, the node is subdivided).
	unsigned distMultRemove;	//!< Relative distance for removing chunks (when distance camera-node's center > distMultRemove, the node is deleted)
	bool transparency;

	void createTree(QuadNode<Chunk*>* node, size_t depth);			//!< Recursive
	bool fullConstChunks(unsigned treeIndex);						//!< Check that visible nodes in a tree are ready (loaded).
	void changeRenders(unsigned treeIndex, bool renderMode);		//!< Change number of renders of all visible chunks in a tree.
	void putToLastDraw(unsigned treeIndex);							//!< Draw last all visible leaf chunks in a tree
	void removeFarChunks(unsigned relDist, glm::vec3 camPosNow);	//!< Remove those chunks that are too far from camera
	glm::vec4 getChunkIDs(unsigned parentID, unsigned depth);

	virtual glm::vec3 getChunkCenter(Chunk* chunk);					//!< Get chunk's center
	virtual QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID) = 0;
	virtual std::tuple<float, float, float> closestCenter() = 0;	//!< Find closest center to the camera of the biggest chunk (i.e. lowest level chunk).

	void restartSideDepths(QuadNode<Chunk*>* node);					//!< Recursive (Preorder traversal). Set side depths of all nodes in the tree to 0.
	void updateChunksSideDepths(QuadNode<Chunk*>* node);			//!< Breath-first search for computing the depth that each side of the chunk must fit.
	void updateChunksSideDepths_help(std::list<QuadNode<Chunk*>*>& queue, QuadNode<Chunk*>* currentNode); //!< Helper method. Computes the depth of each side of a chunk based on adjacent chunks (right and down).

	void resetVisibility(QuadNode<Chunk*>* node);					//!< Recursive (Preorder traversal). Traverse tree and set all leaves (chunks) as visible.
	virtual void updateVisibilityState();							//!< Update some parameters used in isVisible().
	virtual bool isVisible(const Chunk* chunk);						//!< Check if a given chunk is visible (if not, it's not rendered).
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
	TerrainGrid(Renderer* renderer, Noiser* noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, bool transparency);
	~TerrainGrid() { if (noiseGen) delete noiseGen; }

private:
	Noiser* noiseGen;
	QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID) override;
	std::tuple<float, float, float> closestCenter() override;
};


class PlanetGrid : public DynamicGrid
{
public:
	PlanetGrid(Renderer* renderer, std::shared_ptr<Noiser> noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter, bool transparency);
	virtual ~PlanetGrid() { }

	float getRadius();

protected:
	std::shared_ptr<Noiser> noiseGen;
	float radius;
	glm::vec3 nucleus;
	glm::vec3 cubePlane;
	glm::vec3 cubeSideCenter;
	float dotHorizon;			// Minimum dot product. Chunks with lower dotHorizon aren't rendered

	virtual QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID) override;
	std::tuple<float, float, float> closestCenter() override;
	void updateVisibilityState() override;
	bool isVisible(const Chunk* chunk) override;
	glm::vec3 getChunkCenter(Chunk* chunk) override;
};


class SphereGrid : public PlanetGrid
{
public:
	SphereGrid(Renderer* renderer, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter, bool transparency);
	
	QuadNode<Chunk*>* getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID) override;
};


// Planet ----------------------------------------------------------------

//enum dir{ pX, nX, pY, nY, pZ, nZ };

/**
	Six PlanetGrid objects that make up a planet and update the internal SphericalChunk objects depending upon camera position.
	1. Constructor
		- DynamicGrid.constructor()
	2. addResources() (once)
		- DynamicGrid.addTextures()
		- DynamicGrid.addShaders()
	3. updateState() (each frame)
		- DynamicGrid.updateTree()
		- DynamicGrid.updateUBOs()
*/
class Planet
{
public:
	Planet(Renderer* renderer, std::shared_ptr<Noiser> noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency);
	virtual ~Planet();

	void addResources(const std::vector<ShaderLoader>& shaders, const std::vector<TextureLoader>& textures);							//!< Add textures and shader
	void updateState(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj, const LightSet& lights, float frameTime, float groundHeight);	//!< Update tree and UBOs
	void toLastDraw();
	float getGroundHeight(const glm::vec3& camPos);
	void getActiveLeafChunks(std::vector<Chunk*>& dest, unsigned depth) const;
	std::shared_ptr<Noiser> getNoiseGen() const;
	float getSphereArea();							//!< Given planet radius, get sphere's area
	void printCounts();

	const float radius;
	const glm::vec3 nucleus;

protected:
	std::shared_ptr<Noiser> noiseGen;
	PlanetGrid* planetGrid_pZ;
	PlanetGrid* planetGrid_nZ;
	PlanetGrid* planetGrid_pY;
	PlanetGrid* planetGrid_nY;
	PlanetGrid* planetGrid_pX;
	PlanetGrid* planetGrid_nX;

	bool readyForUpdate;

	virtual float callBack_getFloorHeight(const glm::vec3& pos);	//!< Callback example

	//friend GrassSystem;
};


class Sphere : public Planet
{
	float callBack_getFloorHeight(const glm::vec3& pos) override;	//!< Callback example

public:
	Sphere(Renderer* renderer, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency);
	~Sphere();
};


// Grass ----------------------------------------------------------------------

bool grassSupported_callback(const glm::vec3& pos, float groundSlope);

class GrassSystem
{
public:
	GrassSystem(Renderer& renderer, float maxDist, bool(*grassSupported_callback)(const glm::vec3& pos, float groundSlope) = grassSupported_callback);
	~GrassSystem();

	void createGrassModel(std::vector<ShaderLoader>& shaders, std::vector<TextureLoader>& textures, const LightSet* lights);
	void toLastDraw();

protected:
	Renderer& renderer;
	modelIter grassModel;

	std::vector<glm::vec3> pos;     //!< position
	std::vector<glm::vec4> rot;     //!< rotation quaternions
	std::vector<glm::vec3> sca;		//!< scaling
	std::vector<float> slp;			//!< ground slope
	//std::vector<int> index;		//!< Indices (this is shorted). Represent the sorted order of the other lists (pos, rot, sca, slp).

	//Quicksort_distVec3_index sorter;

	bool modelOrdered;
	glm::vec3 camPos, camDir;
	float pi, fov;

	float maxDist;							//!< Max. rendering distance

	//virtual void getGrassItems(bool toSort)  = 0;
	//virtual bool renderRequired() = 0;								//!< Evaluated each frame. Function used by this class for evaluating sub-class related conditions and forcing grass rendering.
	bool(*grassSupported) (const glm::vec3& pos, float groundSlope);	//!< Evaluated each grass. Callback used by the client for evaluating world-related conditions and forcing grass rendering.

	bool withinFOV(const glm::vec3& itemPos, const glm::vec3& camPos, const glm::vec3& camDir, float fov);
};

class GrassSystem_XY : public GrassSystem
{
public:
	GrassSystem_XY(Renderer& renderer, float step, float side, float maxDist);
	~GrassSystem_XY();

protected:
	float whiteNoise[30][30];
	float step;				//!< step size
	float side;				//!< steps per side

	void getGrassItems();
	//bool renderRequired() override;
};

class GrassSystem_planet : public GrassSystem
{
public:
	GrassSystem_planet(Renderer& renderer, float maxDist, unsigned minDepth);
	~GrassSystem_planet();

	void updateState(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camDir, float fov, const LightSet& lights, const Planet& planet, float time);

protected:
	float whiteNoise[15][15][15];	// Rotation angles for grass bunchs to be randomly rotated
	std::vector<Chunk*> chunks;
	unsigned minDepth;				//!< Used chunks have this depth or more
	unsigned chunksCount;			//!< Number of chunks used in the last grass rendering

	glm::vec4 getLatLonRotQuat(glm::vec3& normal);					//!< Rotation angles for grass to be vertically planted on ground (based on normal under camera).
	glm::vec3 getProjectionOnPlane(glm::vec3& normal, glm::vec3& vec);
	bool renderRequired(const Planet& planet);						//!< Evaluated each frame. Detect whether new chunks are available. If so, render the grass of these chunks.
	void getGrassItems(const Planet& planet);
		void getGrassItems_fullGrass(const Planet& planet);
		void getGrassItems_average(const Planet& planet);

	unsigned maxPosSize = 0;	// for testing 
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
