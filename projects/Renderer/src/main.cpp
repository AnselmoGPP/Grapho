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
		> Terrain
		> Modify terrainGenerator for it to have some state (noiseSet...) and generate buffers outside itself
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
		> Scene plane: Draw in front of some rendering (used for skybox or weapons)
		Shading stuff (lights, diffuse, ...)
		Make classes more secure (hide sensitive variables)
		Parallel loading (many threads)
		When passing vertex data directly, should I copy it or pass by reference? Ok, a ref is passed to Renderer, which passes a ref to modelData, which copies data in a vector, and later in a VkBuffer. Could we avoid the copy in a vector?
		> fsUBO implementation in shader
		> Many renders: Now, UBO is passes many times, so View and Projection matrix are redundant. 
		> Generalize loadModel() (VertexPCT, etc.) 
		> Can uniforms be destroyed within the UBO class whithout making user responsible for destroying before creating 
		> Check that different operations work (add/remove renders, add/erase model, 0 renders, ... do it with different primitives)
		X VkDrawIndex instanceCount -> check this way of multiple renderings
		X In a single draw, draw skybox from one mesh and many textures.
	
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

// TODO now: UBO for fragment shader / Shared textures / Reorganize 2nd thread / Parallel thread manager

#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "toolkit.hpp"
#include "geometry.hpp"

//===============================================================================

// File's paths
#if defined(__unix__)
const std::string shaders_dir("../../../projects/Renderer/shaders/SPIRV/");
const std::string textures_dir("../../../textures/");
#elif _WIN64 || _WIN32
const std::string SHADERS_DIR("../../../projects/Renderer/shaders/SPIRV/");
const std::string MODELS_DIR("../../../models/");
const std::string TEXTURES_DIR("../../../textures/");
#endif

// Models & textures
std::map<std::string, modelIterator> assets;	// Model iterators
std::map<std::string, texIterator> textures;	// Texture iterators

// Sun & light
float dayTime = 15.00;
float sunDist = 500;
float sunAngDist = 3.14/10;
Light sun;

// Others
int gridStep = 50;
ifOnce check;			// LOOK implement as functor (function with state)

std::vector<VertexPC> v_points = {
	VertexPC(glm::vec3(-10, -10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(0, -10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(10, -10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(-10,   0,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(0,   0,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(10,   0,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(-10,  10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(0,  10,  10), glm::vec3(1.f, 0.f, 0.f)),
	VertexPC(glm::vec3(10,  10,  10), glm::vec3(1.f, 0.f, 0.f))
};

// Terrain
terrainGenerator terrGen;
noiseSet noiser( 
	5, 1.5, 0.28f,					// Octaves, Lacunarity, Persistance
	1, 150,							// Scale, Multiplier
	2,								// Curve degree
	0, 0,							// X offset, Y offset
	FastNoiseLite::NoiseType_Perlin,// Noise type
	false,							// Random offset
	0);								// Seed

// Data to update
long double frameTime;
size_t fps;
size_t maxfps;
glm::vec3 pos;

// Functions declarations
void update		(Renderer& r);		// Update model's MM (model matrix) each frame
void setReticule(Renderer& app);
void setPoints	(Renderer& app);
void setAxis	(Renderer& app);
void setGrid	(Renderer& app);
void setSkybox	(Renderer& app);
void setCottage	(Renderer& app);
void setRoom	(Renderer& app);
void setSun		(Renderer& app);
void setTerrain	(Renderer& app);

//===============================================================================

int main(int argc, char* argv[])
{
	// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
	Renderer app(update);		
	
	std::cout << "------------------------------" << std::endl;
	TimerSet time;
	std::cout << time.getDate() << std::endl;

	setPoints(app);
	setAxis(app);
	setGrid(app);
	setSkybox(app);
	setCottage(app);
	setRoom(app);
	setTerrain(app);
	setSun(app);
	setReticule(app);

	app.run();		// Start rendering
	
	return EXIT_SUCCESS;
}


void update(Renderer& r)
{
	frameTime	= r.getTimer().getTime();
	fps			= r.getTimer().getFPS();
	maxfps		= r.getTimer().getMaxPossibleFPS();
	pos			= r.getCamera().Position;

	if (check.ifBigger(frameTime, 5)) std::cout << "5 seconds in" << std::endl;

	if (assets.find("skyBox") != assets.end())
		assets["skyBox"]->setMM(0, 0, modelMatrix(glm::vec3(2048.0f, 2048.0f, 2048.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(pos.x, pos.y, pos.z)));

	if (assets.find("sun") != assets.end())
		assets["sun"]->setMM(0, 0, sunMM(pos, dayTime, sunDist, sunAngDist));

	if (assets.find("grid") != assets.end())
		assets["grid"]->setMM(0, 0, modelMatrix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(gridStep*((int)pos.x/gridStep), gridStep*((int)pos.y/gridStep), 0.0f)));

	if (assets.find("cottage") != assets.end())
		assets["cottage"]->setMM(0, 0, modelMatrix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(90.0f, frameTime * 45.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
}


void setSun(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	std::vector<VertexPT> v_sun;
	std::vector<uint32_t> i_sun;
	size_t numVertex = getPlane(v_sun, i_sun, 1.f, 1.f);		// LOOK dynamic adjustment of reticule size when window is resized

	std::vector<Texture> textures = { Texture((TEXTURES_DIR + "Sun/sun2_1.png").c_str()) };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), numVertex, v_sun.data(), i_sun, true);

	assets["sun"] = app.newModel(
		1, primitiveTopology::triangle,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		textures,
		(SHADERS_DIR + "v_sunPT.spv").c_str(),
		(SHADERS_DIR + "f_sunPT.spv").c_str(),
		true);

	sun.setDirectional(sunLightDirection(dayTime), glm::vec3(.1f, .1f, .1f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(.5f, .5f, .5f));
}

void setReticule(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	std::vector<VertexPT> v_ret;
	std::vector<uint32_t> i_ret;
	size_t numVertex = getPlaneNDC(v_ret, i_ret, 0.2f, 0.2f);		// LOOK dynamic adjustment of reticule size when window is resized

	std::vector<Texture> textures = { Texture((TEXTURES_DIR + "HUD/reticule_1.png").c_str()) };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), numVertex, v_ret.data(), i_ret, true);

	assets["reticule"] = app.newModel(
		1, primitiveTopology::triangle,
		vertexLoader,
		noUBO,
		noUBO,
		textures,
		(SHADERS_DIR + "v_hudPT.spv").c_str(),
		(SHADERS_DIR + "f_hudPT.spv").c_str(),
		true);
}

void setPoints(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	Icosahedron icos;	// Just created for calling destructor, which applies a multiplier.
	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), Icosahedron::icos.size()/6, Icosahedron::icos.data(), noIndices, false);

	assets["points"] = app.newModel(
		1, primitiveTopology::point,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		noTextures,
		(SHADERS_DIR + "v_pointPC.spv").c_str(),
		(SHADERS_DIR + "f_pointPC.spv").c_str(),
		false);

	//assets["points"]->setMM(0, modelMatrix());
}

void setPoints2(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), 9, v_points.data(), noIndices, false);

	assets["points"] = app.newModel(
		1, primitiveTopology::point,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		noTextures,
		(SHADERS_DIR + "v_pointPC.spv").c_str(),
		(SHADERS_DIR + "f_pointPC.spv").c_str(),
		false);

	//assets["points"]->setMM(0, modelMatrix());
}

void setAxis(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	std::vector<VertexPC> v_axis;
	std::vector<uint32_t> i_axis;
	size_t numVertex = getAxis(v_axis, i_axis, 100, 0.8);

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), numVertex, v_axis.data(), i_axis, true);

	assets["axis"] = app.newModel(
		1, primitiveTopology::line,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		noTextures,
		(SHADERS_DIR + "v_linePC.spv").c_str(),
		(SHADERS_DIR + "f_linePC.spv").c_str(),
		false);

	//assets["axis"]->setMM(0, modelMatrix());
}

void setGrid(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	std::vector<VertexPC> v_grid;
	std::vector<uint32_t> i_grid;
	size_t numVertex = getGrid(v_grid, i_grid, gridStep, 50, glm::vec3(0.1, 0.1, 0.6));

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), numVertex, v_grid.data(), i_grid, true);

	assets["grid"] = app.newModel(
		1, primitiveTopology::line,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		noTextures,
		(SHADERS_DIR + "v_linePC.spv").c_str(),
		(SHADERS_DIR + "f_linePC.spv").c_str(),
		false);

	//assets["grid"]->setMM(0, modelMatrix());
}

void setSkybox(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	std::vector<Texture> textures = { Texture((TEXTURES_DIR + "sky_box/space1.jpg").c_str()) };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), 14, v_cube.data(), i_inCube, false);

	assets["skyBox"] = app.newModel(
		1, primitiveTopology::triangle,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		textures,
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		false);
}

void setCottage(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	// Add a model to render. An iterator is returned (modelIterator). Save it for updating model data later.
	std::vector<Texture> textures = { Texture((TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str()) };
	VertexLoader* vertexLoader;
	
	vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (MODELS_DIR + "cottage_obj.obj").c_str());

	assets["cottage"] = app.newModel(
		0, primitiveTopology::triangle,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		textures,
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		false);

	// Delete a model you passed previously.
	app.deleteModel(assets["cottage"]);

	vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (MODELS_DIR + "cottage_obj.obj").c_str());

	assets["cottage"] = app.newModel(
		1, primitiveTopology::triangle,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		textures,
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		false);
}

void setRoom(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	std::vector<Texture> textures = { Texture((TEXTURES_DIR + "viking_room.png").c_str()) };

	VertexLoader* vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (MODELS_DIR + "viking_room.obj").c_str());

	assets["room"] = app.newModel(
		2, primitiveTopology::triangle,
		vertexLoader,
		UBOtype(1, 1, 1, 0),
		noUBO,
		textures,
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		false );

	assets["room"]->setMM(0, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, -90.0f), glm::vec3(0.0f, -50.0f, 3.0f)));
	assets["room"]->setMM(1, 0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -80.0f, 3.0f)));
	//assets["room"]->setMM(2, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f,  90.0f), glm::vec3(30.0f, -80.0f, 3.0f)));
	//assets["room"]->setMM(3, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 180.0f), glm::vec3(30.0f, -50.0f, 3.0f)));
}

void setTerrain(Renderer& app)
{
	std::cout << "> " << __func__ << std::endl;

	terrGen.computeTerrain(noiser, 0, 0, 5, 20, 20, 1.f);

	std::vector<Texture> textures = 
	{ 
		Texture((TEXTURES_DIR + "squares.png").c_str()),
		Texture((TEXTURES_DIR + "grass.png").c_str())
	};

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 1), terrGen.getNumVertex(), terrGen.vertex,	terrGen.indices, true);

	assets["terrain"] = app.newModel(
		1, primitiveTopology::triangle,
		vertexLoader,
		UBOtype(1, 1, 1, 1),
		noUBO,//UBOtype(0, 0, 0, 0, 1),
		textures,
		(SHADERS_DIR + "v_terrainPTN.spv").c_str(),
		(SHADERS_DIR + "f_terrainPTN.spv").c_str(),
		false);
}