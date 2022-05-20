/*
	Renderer	< VulkanEnvironment
				< ModelData		< VulkanEnvironment
								< modelConfig	< VulkanEnvironment
												< getModelMatrix callbacks
								< Input			< Camera

	Data passed:
		- Vertex (positions, colors, texture coords, normals...)
		- Indices
		- Descriptors:
			- UBOs (MVP matrices, M matrix for normals...)
			- Textures/maps (diffuse, specular...)
		- Shaders (vertex, fragment...)
*/

/*
	TODO:
		- Axis
		- Sun billboard (transparencies)
		- Terrain
		> Modify NoiseSurface for it to have some state (noiseSet...) and generate buffers outside itself
		Make the renderer a static library
		Add ProcessInput() maybe
		Dynamic states (graphics pipeline)
		Push constants
		Deferred rendering (https://gamedevelopment.tutsplus.com/articles/forward-rendering-vs-deferred-rendering--gamedev-12342)

		UBO of each renders should be stored in a vector-like structure, so there are UBO available for new renders (generated with setRender())
		Destroy Vulkan buffers (UBO) outside semaphores

	Rendering:
		- Vertex struct has pos, color, text coord. Different vertex structs are required.
		- Points, lines, triangles
		- 2D graphics
		- Transparencies
		- Scene plane: Draw in front of some rendering (used for skybox or weapons)
		Make classes more secure (hide sensitive variables)
		Parallel loading (many threads)
		When passing vertex data directly, should I copy it or pass by reference? Ok, a ref is passed to Renderer, which passes a ref to modelData, which copies data in a vector, and later in a VkBuffer. Could we avoid the copy in a vector?
		> Many renders: Now, UBO is passed many times, so View and Projection matrix are redundant. 
		> update(): Projection matrix should be updated only when it has changed
		> Generalize loadModel() (VertexPCT, etc.) 
		> Can uniforms be destroyed within the UBO class whithout making user responsible for destroying before creating 
		> Check that different operations work (add/remove renders, add/erase model, 0 renders, ... do it with different primitives)
		X VkDrawIndex instanceCount -> check this way of multiple renderings
	
		- Allow to update MM immediately after addModel() or addRender()
		- Only dynamic UBOs
		- Start thread since run() (objectAlreadyConstructed)
		- Improve modelData object destruction (call stuff from destructor, and take code out from Renderer)
		Can we take stuff out from thread 2?
		Optimization: Parallel commandBuffer creation (2 or more commandBuffers exist)
		model&commandBuffer mutex, think about it
		Usar numMM o MM.size()?
		Profiling
		Skybox borders (could be fixed with VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)

	Abstract:
		> Textures set (share textures)
		> Descriptor set

	BUGS:
		Sometimes camera continue moving backwards/left indefinetely
		Camera jump when starting to move camera

	Model & Data system:
		Each ModelData could have: Vertices, Color buffers, textures, texture coords, Indices, UBO class, shaders, vertex struct
		Unique elements (always): Vertices, indices, shaders
		Unique elements (sometimes): Color buffer, texture coords,
		Shared elements (sometimes): UBO class, Textures, vertex struct(Vertices, color, textCoords)
*/

// TerrainGrid: Use int instead of floats for map key?
// Am I asking GPU for too large heap space?
// Indices to 16 bytes
// Camera
// Inputs (MVC)
// GUI
// Profiling
// Pass material to Fragment Shader
// learnopengl.com
// Later: Readme.md
// Later: Multithread loading / Reorganize 2nd thread / Parallel thread manager
// Later: Eliminate mutex (mutex_modelsToLoad, mutex_modelsToDelete) (maybe mutex_rendersToSet too) like I did with texture loading/deletion. Check loadingThread()


#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "toolkit.hpp"

#include "terrain.hpp"
#include "common.hpp"

//===============================================================================

// Models & textures
std::map<std::string, modelIterator> assets;	// Model iterators
std::map<std::string, texIterator> textures;	// Texture iterators

// Others
int gridStep = 50;
ifOnce check;			// LOOK implement as functor (function with state)

// Terrain
Noiser noiser(
	FastNoiseLite::NoiseType_Perlin,// Noise type
	4, 1.5, 0.28f,					// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
	1, 50,							// Scale, Multiplier
	0,								// Curve degree
	500, 500, 0,					// XYZ offsets
	4952);							// Seed

Chunk singleChunk(glm::vec3(50, 50, 0), 200, 41, 11);

TerrainGrid terrChunks(noiser, 6400, 21, 7, 2, 1);

// Data to update
long double frameTime;
size_t fps;
size_t maxfps;
glm::vec3 pos;
//Renderer* appPtr;

//===============================================================================

void update(Renderer& rend, glm::mat4 view, glm::mat4 proj);		// Update UBOs each frame

void loadTextures(Renderer& app);

void setPoints(Renderer& app);
void setAxis(Renderer& app);
void setGrid(Renderer& app);
void setSkybox(Renderer& app);
void setCottage(Renderer& app);
void setRoom(Renderer& app);
void setChunk(Renderer& app);
void setChunkSet(Renderer& app);
void setSun(Renderer& app);
void setReticule(Renderer& app);


int main(int argc, char* argv[])
{
	TimerSet time;

	// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
	Renderer app(update, 3);
	
	std::cout << "------------------------------" << std::endl << time.getDate() << std::endl;

	loadTextures(app);

	setPoints(app);
	setAxis(app);
	setGrid(app);
	setSkybox(app);
	setCottage(app);
	setRoom(app);
	//setChunk(app);
	setChunkSet(app);
	setSun(app);
	setReticule(app);

	app.run();		// Start rendering
	
	return EXIT_SUCCESS;
}


void update(Renderer& rend, glm::mat4 view, glm::mat4 proj)
{
	frameTime	= rend.getTimer().getTime();
	fps			= rend.getTimer().getFPS();
	maxfps		= rend.getTimer().getMaxPossibleFPS();
	pos			= rend.getCamera().Position;
	size_t i;
	
	if (check.ifBigger(frameTime, 5))
		if (assets.find("room") != assets.end())
		{
			rend.deleteModel(assets["room"]);		// TEST (in render loop): deleteModel
			assets.erase("room");

			rend.deleteTexture(textures["room"]);	// TEST (in render loop): deleteTexture
			textures.erase("room");
		}

	if (check.ifBigger(frameTime, 6))
	{
		textures["room"] = rend.newTexture((TEXTURES_DIR + "viking_room.png").c_str());	// TEST (in render loop): newTexture
		setRoom(rend);																	// TEST (in render loop): newModel
	}

	if (check.ifBigger(frameTime, 8))
		rend.setRenders(assets["room"], 1);			// TEST (in render loop): setRenders (in range)

	if (check.ifBigger(frameTime, 10))
	{
		rend.setRenders(assets["room"], 4);			// TEST (in render loop): setRenders (out of range)
		assets["room"]->vsDynUBO.setUniform(0, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, -90.0f), glm::vec3(0.0f, -50.0f, 3.0f)));
		assets["room"]->vsDynUBO.setUniform(1, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -80.0f, 3.0f)));
		assets["room"]->vsDynUBO.setUniform(2, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 90.0f), glm::vec3(30.0f, -80.0f, 3.0f)));
		assets["room"]->vsDynUBO.setUniform(3, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 180.0f), glm::vec3(30.0f, -50.0f, 3.0f)));
	}

	// Update UBOs
	if (assets.find("points") != assets.end())
		for (i = 0; i < assets["points"]->vsDynUBO.dynBlocksCount; i++) {
			assets["points"]->vsDynUBO.setUniform(i, 1, view);
			assets["points"]->vsDynUBO.setUniform(i, 2, proj);
		}

	if (assets.find("axis") != assets.end())
		for (i = 0; i < assets["axis"]->vsDynUBO.dynBlocksCount; i++) {
			assets["axis"]->vsDynUBO.setUniform(i, 1, view);
			assets["axis"]->vsDynUBO.setUniform(i, 2, proj);
		}

	if (assets.find("grid") != assets.end())
		for (i = 0; i < assets["grid"]->vsDynUBO.dynBlocksCount; i++) {
			assets["grid"]->vsDynUBO.setUniform(i, 0, modelMatrix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(gridStep * ((int)pos.x / gridStep), gridStep * ((int)pos.y / gridStep), 0.0f)));
			assets["grid"]->vsDynUBO.setUniform(i, 1, view);
			assets["grid"]->vsDynUBO.setUniform(i, 2, proj);
		}

	if (assets.find("skyBox") != assets.end())
		for (i = 0; i < assets["skyBox"]->vsDynUBO.dynBlocksCount; i++) {
			assets["skyBox"]->vsDynUBO.setUniform(i, 0, modelMatrix(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(pos.x, pos.y, pos.z)));
			assets["skyBox"]->vsDynUBO.setUniform(i, 1, view);
			assets["skyBox"]->vsDynUBO.setUniform(i, 2, proj);
		}

	if (assets.find("cottage") != assets.end())
		for (i = 0; i < assets["cottage"]->vsDynUBO.dynBlocksCount; i++) {
			assets["cottage"]->vsDynUBO.setUniform(i, 0, modelMatrix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(90.0f, frameTime * 45.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
			assets["cottage"]->vsDynUBO.setUniform(i, 1, view);
			assets["cottage"]->vsDynUBO.setUniform(i, 2, proj);
		}
		
	if (assets.find("room") != assets.end())
		for (i = 0; i < assets["room"]->vsDynUBO.dynBlocksCount; i++) {
			assets["room"]->vsDynUBO.setUniform(i, 1, view);
			assets["room"]->vsDynUBO.setUniform(i, 2, proj);
		}

	if (assets.find("terrain") != assets.end()) {
		for (i = 0; i < assets["terrain"]->vsDynUBO.dynBlocksCount; i++) {
			assets["terrain"]->vsDynUBO.setUniform(i, 1, view);
			assets["terrain"]->vsDynUBO.setUniform(i, 2, proj);
		}
		assets["terrain"]->fsUBO.setUniform(0, 1, pos);
	}

	singleChunk.updateUBOs(pos, view, proj);

	terrChunks.updateTree(pos);
	terrChunks.updateUBOs(pos, view, proj);

	if (assets.find("sun") != assets.end())
		for (i = 0; i < assets["sun"]->vsDynUBO.dynBlocksCount; i++) {
			assets["sun"]->vsDynUBO.setUniform(i, 0, sunMM(pos, dayTime, 0.5f, sunAngDist));
			assets["sun"]->vsDynUBO.setUniform(i, 1, view);
			assets["sun"]->vsDynUBO.setUniform(i, 2, proj);
		}
}

void loadTextures(Renderer& app)
{
	textures["skybox"] = app.newTexture((TEXTURES_DIR + "sky_box/space1.jpg").c_str());
	textures["cottage"] = app.newTexture((TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str());
	textures["room"] = app.newTexture((TEXTURES_DIR + "viking_room.png").c_str());
	textures["squares"] = app.newTexture((TEXTURES_DIR + "squares.png").c_str());
	textures["grass"] = app.newTexture((TEXTURES_DIR + "grass.png").c_str());
	textures["sun"] = app.newTexture((TEXTURES_DIR + "Sun/sun2_1.png").c_str());
	textures["reticule"] = app.newTexture((TEXTURES_DIR + "HUD/reticule_1.png").c_str());

	app.deleteTexture(textures["skybox"]);												// TEST (before render loop): deleteTexture
	textures["skybox"] = app.newTexture((TEXTURES_DIR + "sky_box/space1.jpg").c_str());	// TEST (before render loop): newTexture

	// <<< You could build materials (make sets of textures) here
	// <<< Then, user could make sets of materials and send them to a modelObject
}


void setPoints(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	Icosahedron icos(30.f);	// Just created for calling destructor, which applies a multiplier.
	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), Icosahedron::icos.size() / 6, Icosahedron::icos.data(), noIndices, false);

	assets["points"] = app.newModel(
		1, 1, primitiveTopology::point,
		vertexLoader,
		UBOconfig(1, MMsize, VMsize, PMsize),
		noUBO,
		noTextures,
		(SHADERS_DIR + "v_pointPC.spv").c_str(),
		(SHADERS_DIR + "f_pointPC.spv").c_str(),
		false);

	assets["points"]->vsDynUBO.setUniform(0, 0, modelMatrix());
}

void setAxis(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<VertexPC> v_axis;
	std::vector<uint32_t> i_axis;
	size_t numVertex = getAxis(v_axis, i_axis, 100, 0.8);

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), numVertex, v_axis.data(), i_axis, true);

	assets["axis"] = app.newModel(
		2, 1, primitiveTopology::line,
		vertexLoader,
		UBOconfig(1, MMsize, VMsize, PMsize),
		noUBO,
		noTextures,
		(SHADERS_DIR + "v_linePC.spv").c_str(),
		(SHADERS_DIR + "f_linePC.spv").c_str(),
		false);

	assets["axis"]->vsDynUBO.setUniform(0, 0, modelMatrix());
}

void setGrid(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	
	std::vector<VertexPC> v_grid;
	std::vector<uint32_t> i_grid;
	size_t numVertex = getGrid(v_grid, i_grid, gridStep, 50, 0, glm::vec3(0.1, 0.1, 0.6));

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), numVertex, v_grid.data(), i_grid, true);

	assets["grid"] = app.newModel(
		1, 1, primitiveTopology::line,
		vertexLoader,
		UBOconfig(1, MMsize, VMsize, PMsize),
		noUBO,
		noTextures,
		(SHADERS_DIR + "v_linePC.spv").c_str(),
		(SHADERS_DIR + "f_linePC.spv").c_str(),
		false);
}

void setSkybox(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<texIterator> usedTextures = { textures["skybox"] };
	//std::vector<Texture> textures = { Texture((TEXTURES_DIR + "sky_box/space1.jpg").c_str()) };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), 14, v_cube.data(), i_inCube, false);

	assets["skyBox"] = app.newModel(
		0, 1, primitiveTopology::triangle,
		vertexLoader,
		UBOconfig(1, MMsize, VMsize, PMsize),
		noUBO,
		usedTextures,
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		false);
}

void setCottage(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	// Add a model to render. An iterator is returned (modelIterator). Save it for updating model data later.
	std::vector<texIterator> usedTextures = { textures["cottage"] };
	//std::vector<Texture> textures = { Texture((TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str()) };

	VertexLoader* vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (MODELS_DIR + "cottage_obj.obj").c_str());

	assets["cottage"] = app.newModel(			// TEST (before render loop): newModel
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		UBOconfig(1, MMsize, VMsize, PMsize),
		noUBO,
		usedTextures,
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		false);

	// Delete a model you passed previously.
	app.deleteModel(assets["cottage"]);			// TEST (before render loop): deleteModel

	vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (MODELS_DIR + "cottage_obj.obj").c_str());

	assets["cottage"] = app.newModel(
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		UBOconfig(1, MMsize, VMsize, PMsize),
		noUBO,
		usedTextures,
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		false);
}

void setRoom(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<texIterator> usedTextures = { textures["room"] };
	//std::vector<Texture> textures = { Texture((TEXTURES_DIR + "viking_room.png").c_str()) };

	VertexLoader* vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (MODELS_DIR + "viking_room.obj").c_str());

	assets["room"] = app.newModel(
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		UBOconfig(2, MMsize, VMsize, PMsize),
		noUBO,
		usedTextures,
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		false);

	app.setRenders(assets["room"], 2);	// TEST (before render loop): setRenders
	app.setRenders(assets["room"], 3);	// TEST(in render loop) : setRenders(out of range)

	assets["room"]->vsDynUBO.setUniform(0, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, -90.0f), glm::vec3(0.0f, -50.0f, 3.0f)));
	assets["room"]->vsDynUBO.setUniform(1, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -80.0f, 3.0f)));
	assets["room"]->vsDynUBO.setUniform(2, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 90.0f), glm::vec3(30.0f, -80.0f, 3.0f)));
	//assets["room"]->vsDynUBO.setUniform(3, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 180.0f), glm::vec3(30.0f, -50.0f, 3.0f)));
}

void setChunk(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<texIterator> usedTextures = { textures["squares"], textures["grass"] };

	singleChunk.computeTerrain(noiser, true, 1.f);
	singleChunk.render(&app, usedTextures, nullptr);
}

void setChunkSet(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<texIterator> usedTextures = { textures["squares"], textures["grass"] };
	terrChunks.addTextures(usedTextures);
	terrChunks.addApp(app);

	//terrChunks.updateTree(glm::vec3(0,0,0));
}

void setSun(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<VertexPT> v_sun;
	std::vector<uint32_t> i_sun;
	size_t numVertex = getPlane(v_sun, i_sun, 1.f, 1.f);		// LOOK dynamic adjustment of reticule size when window is resized

	std::vector<texIterator> usedTextures = { textures["sun"] };
	//std::vector<Texture> textures = { Texture((TEXTURES_DIR + "Sun/sun2_1.png").c_str()) };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), numVertex, v_sun.data(), i_sun, true);

	assets["sun"] = app.newModel(
		0, 1, primitiveTopology::triangle,
		vertexLoader,
		UBOconfig(1, MMsize, VMsize, PMsize),
		noUBO,
		usedTextures,
		(SHADERS_DIR + "v_sunPT.spv").c_str(),
		(SHADERS_DIR + "f_sunPT.spv").c_str(),
		true);

	//sun.setDirectional(sunLightDirection(dayTime), glm::vec3(.1f, .1f, .1f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(.5f, .5f, .5f));
}

void setReticule(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<VertexPT> v_ret;
	std::vector<uint32_t> i_ret;
	size_t numVertex = getPlaneNDC(v_ret, i_ret, 0.2f, 0.2f);		// LOOK dynamic adjustment of reticule size when window is resized

	std::vector<texIterator> usedTextures = { textures["reticule"] };
	//std::vector<Texture> textures = { Texture((TEXTURES_DIR + "HUD/reticule_1.png").c_str()) };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), numVertex, v_ret.data(), i_ret, true);

	assets["reticule"] = app.newModel(
		2, 1, primitiveTopology::triangle,
		vertexLoader,
		UBOconfig(1),
		noUBO,
		usedTextures,
		(SHADERS_DIR + "v_hudPT.spv").c_str(),
		(SHADERS_DIR + "f_hudPT.spv").c_str(),
		true);
}
