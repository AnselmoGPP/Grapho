/*
	Renderer	< VulkanEnvironment
				< ModelData		< VulkanEnvironment
								< modelConfig	< VulkanEnvironment
												< getModelMatrix callbacks
								< Input			< Camera

	set of modelConfig (callbacks + paths) > Renderer > set of ModelData

	Data passed:
		- Vertex data:
			- Vertex coordinates
			- Color
			- Texture coordinates
			- Normals
		- Indices
		- Descriptor set:
			- UBO (MVP matrices, normal matrix...)
			- Texture samplers
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
		Bug: Some drag click produces camera jump

		UBO of each renders should be stored in a vector-like structure, so there are UBO available for new renders (generated with setRender())
		Destroy Vulkan buffers (UBO) outside semaphores

	Rendering:
		- Vertex struct has pos, color, text coord. Different vertex structs are required.
		- Points, lines, triangles
		- 2D graphics
		- Transparencies
		Draw in front of some rendering (used for weapons)
		Shading stuff (lights, diffuse, ...)
		Make classes more secure (hide sensitive variables)
		Parallel loading (many threads)
		When passing vertex data directly, should I copy it or pass by reference? Ok, a ref is passed to Renderer, which passes a ref to modelData, which copies data in a vector, and later in a VkBuffer. Could we avoid the copy in a vector?
		> Generalize VertexPCT in loadModel()
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

	BUGS:
		Sometimes camera continue moving backwards/left indefinetely

	Model & Data system:
		Each ModelData could have: Vertices, Color buffers, textures, texture coords, Indices, UBO class, shaders, vertex struct
		Unique elements (always): Vertices, indices, shaders
		Unique elements (sometimes): Color buffer, texture coords,
		Shared elements (sometimes): UBO class, Textures, vertex struct(Vertices, color, textCoords)
*/

/*
	Render same model with different descriptors
		- You technically don't have multiple uniform buffers; you just have one. But you can use the offset(s) provided to vkCmdBindDescriptorSets
		to shift where in that buffer the next rendering command(s) will get their data from. Basically, you rebind your descriptor sets, but with
		different pDynamicOffset array values.
		- Your pipeline layout has to explicitly declare those descriptors as being dynamic descriptors. And every time you bind the set, you'll need
		to provide the offset into the buffer used by that descriptor.
		- More: https://stackoverflow.com/questions/45425603/vulkan-is-there-a-way-to-draw-multiple-objects-in-different-locations-like-in-d
*/

#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "toolkit.hpp"
#include "data.hpp"
#include "geometry.hpp"

std::map<std::string, modelIterator> assets;
int gridStep = 50;
ifOnce check;			// LOOK implement as functor (function with state)

float dayTime = 15.00;
float sunDist = 500;
float sunAngDist = 3.14/10;

terrainGenerator terrGen;
noiseSet noiser( 
	5, 1.5, 0.28f,					// Octaves, Lacunarity, Persistance
	1, 150,							// Scale, Multiplier
	2,								// Curve degree
	0, 0,							// X offset, Y offset
	FastNoiseLite::NoiseType_Perlin,// Noise type
	false,							// Random offset
	0);								// Seed

void update		(Renderer& r);
void setReticule(Renderer& app);
void setPoints	(Renderer& app);
void setAxis	(Renderer& app);
void setGrid	(Renderer& app);
void setSkybox	(Renderer& app);
void setCottage	(Renderer& app);
void setRoom	(Renderer& app);
void setFloor	(Renderer& app);
void setSun		(Renderer& app);
void setTerrain	(Renderer& app);


int main(int argc, char* argv[])
{
	// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
	Renderer app(update);		

	std::cout << "------------------------------" << std::endl;

	//setPoints(app);
	setAxis(app);
	setGrid(app);
	setSkybox(app);
	//setCottage(app);
	setRoom(app);
	//setFloor(app);
	setTerrain(app);

	setSun(app);
	setReticule(app);

	app.run();		// Start rendering
	
	return EXIT_SUCCESS;
}


// Update model's model matrix each frame
void update(Renderer& r)
{
	long double time	= r.getTimer().getTime();
	size_t fps			= r.getTimer().getFPS();
	size_t maxfps		= r.getTimer().getMaxPossibleFPS();
	glm::vec3 pos		= r.getCamera().Position;


	if (check.ifBigger(time, 5)) std::cout << "5 seconds in" << std::endl;

	if (assets.find("grid") != assets.end())
		assets["grid"]->setUBO(0, modelMatrix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(gridStep*((int)pos.x/gridStep), gridStep*((int)pos.y/gridStep), 0.0f)));

	if (assets.find("skyBoxX") != assets.end())
	{
		assets["skyBoxX" ]->setUBO(0, modelMatrix(glm::vec3(2048.0f, 2048.0f, 2048.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(pos.x, pos.y, pos.z)));
		assets["skyBoxY" ]->setUBO(0, modelMatrix(glm::vec3(2048.0f, 2048.0f, 2048.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(pos.x, pos.y, pos.z)));
		assets["skyBoxZ" ]->setUBO(0, modelMatrix(glm::vec3(2048.0f, 2048.0f, 2048.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(pos.x, pos.y, pos.z)));
		assets["skyBox-X"]->setUBO(0, modelMatrix(glm::vec3(2048.0f, 2048.0f, 2048.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(pos.x, pos.y, pos.z)));
		assets["skyBox-Y"]->setUBO(0, modelMatrix(glm::vec3(2048.0f, 2048.0f, 2048.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(pos.x, pos.y, pos.z)));
		assets["skyBox-Z"]->setUBO(0, modelMatrix(glm::vec3(2048.0f, 2048.0f, 2048.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(pos.x, pos.y, pos.z)));
	}

	if (assets.find("cottage") != assets.end())
		assets["cottage"]->setUBO(0, modelMatrix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(90.0f, time * 45.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

	if (assets.find("sun") != assets.end())
		assets["sun"]->setUBO(0, sunMM(pos, dayTime, sunDist, sunAngDist));


	//if (time > 5 && cottageLoaded)
	//{
	//	r.setRenders(assets["cottage"], 0);
	//	cottageLoaded = false;
	//}
/*
	if (time > 5 && !check1)
	{
		r.setRenders(assets["room"], 4);
		assets["room"]->setUBO(0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, -90.0f), glm::vec3( 0.0f, -50.0f, 3.0f)));
		assets["room"]->setUBO(1, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f,   0.0f), glm::vec3( 0.0f, -80.0f, 3.0f)));
		assets["room"]->setUBO(2, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f,  90.0f), glm::vec3(30.0f, -80.0f, 3.0f)));
		assets["room"]->setUBO(3, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 180.0f), glm::vec3(30.0f, -50.0f, 3.0f)));
		check1 = true;
	}
	else if (time > 10 && !check2)
	{
		 r.deleteModel(assets["room"]);
		//r.setRenders(assets["room"], 3);
		check2 = true;
	}
	
	 if (time > 5)
	 {
		 //assets["room"]->setUBO(0, room1_MM(0));
		 //assets["room"]->setUBO(1, room2_MM(0));
		 //assets["room"]->setUBO(2, room3_MM(0));
		 //assets["room"]->setUBO(3, room4_MM(0));
		 //assets["room"]->setUBO(4, room5_MM(0));
	 }
*/		 

}

void setSun(Renderer& app)
{
	std::vector<VertexPT> v_sun;
	std::vector<uint32_t> i_sun;
	size_t numVertex = getPlane(v_sun, i_sun, 1.f, 1.f);		// LOOK dynamic adjustment of reticule size when window is resized

	assets["sun"] = app.newModel( 1,
		MVPN,
		VertexType(PT, 1, 0, 1), numVertex, v_sun.data(),
		&i_sun,
		(TEXTURES_DIR + "Sun/sun2_1.png").c_str(),
		(SHADERS_DIR + "v_sunPT.spv").c_str(),
		(SHADERS_DIR + "f_sunPT.spv").c_str(),
		primitiveTopology::triangle,
		true);
}

void setReticule(Renderer& app)
{
	std::vector<VertexPT> v_ret;
	std::vector<uint32_t> i_ret;
	size_t numVertex = getPlaneNDC(v_ret, i_ret, 0.2f, 0.2f);		// LOOK dynamic adjustment of reticule size when window is resized

	assets["reticule"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPT), 1, 0, 1), numVertex, v_ret.data(),
		&i_ret,
		(TEXTURES_DIR + "HUD/reticule_1.png").c_str(),
		(SHADERS_DIR + "v_hudPT.spv").c_str(),
		(SHADERS_DIR + "f_hudPT.spv").c_str(),
		primitiveTopology::triangle,
		true );
}

void setPoints(Renderer& app)
{
	assets["points"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPC), 1, 1, 0), 9, v_points.data(),
		nullptr,
		"",
		(SHADERS_DIR + "v_pointPC.spv").c_str(),
		(SHADERS_DIR + "f_pointPC.spv").c_str(),
		primitiveTopology::point);

	//assets["points"]->setUBO(0, modelMatrix());
}

void setAxis(Renderer& app)
{
	std::vector<VertexPC> v_axis;
	std::vector<uint32_t> i_axis;
	size_t numVertex = getAxis(v_axis, i_axis, 100, 0.8);

	assets["axis"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPC), 1, 1, 0), numVertex, v_axis.data(),
		&i_axis,
		"",
		(SHADERS_DIR + "v_linePC.spv").c_str(),
		(SHADERS_DIR + "f_linePC.spv").c_str(),
		primitiveTopology::line);

	//assets["axis"]->setUBO(0, modelMatrix());
}

void setGrid(Renderer& app)
{
	std::vector<VertexPC> v_grid;
	std::vector<uint32_t> i_grid;
	size_t numVertex = getGrid(v_grid, i_grid, gridStep, 6, glm::vec3(0.1, 0.1, 0.6));

	assets["grid"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPC), 1, 1, 0), numVertex, v_grid.data(),
		&i_grid,
		"",
		(SHADERS_DIR + "v_linePC.spv").c_str(),
		(SHADERS_DIR + "f_linePC.spv").c_str(),
		primitiveTopology::line);

	//assets["grid"]->setUBO(0, modelMatrix());
}

void setSkybox(Renderer& app)
{
	assets["skyBoxX"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPT), 1, 0, 1), 4, v_posx.data(),
		&i_square,
		(TEXTURES_DIR + "sky_box/space1_posx.jpg").c_str(),
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		primitiveTopology::triangle);

	assets["skyBoxY"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPT), 1, 0, 1), 4, v_posy.data(),
		&i_square,
		(TEXTURES_DIR + "sky_box/space1_posy.jpg").c_str(),
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		primitiveTopology::triangle);

	assets["skyBoxZ"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPT), 1, 0, 1), 4, v_posz.data(),
		&i_square,
		(TEXTURES_DIR + "sky_box/space1_posz.jpg").c_str(),
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		primitiveTopology::triangle);

	assets["skyBox-X"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPT), 1, 0, 1), 4, v_negx.data(),
		&i_square,
		(TEXTURES_DIR + "sky_box/space1_negx.jpg").c_str(),
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		primitiveTopology::triangle);

	assets["skyBox-Y"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPT), 1, 0, 1), 4, v_negy.data(),
		&i_square,
		(TEXTURES_DIR + "sky_box/space1_negy.jpg").c_str(),
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		primitiveTopology::triangle);

	assets["skyBox-Z"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPT), 1, 0, 1), 4, v_negz.data(),
		&i_square,
		(TEXTURES_DIR + "sky_box/space1_negz.jpg").c_str(),
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		primitiveTopology::triangle);
}

void setCottage(Renderer& app)
{
	// Add a model to render. An iterator is returned (modelIterator). Save it for updating model data later.
	assets["cottage"] = app.newModel( 0,
		MVP,
		(MODELS_DIR   + "cottage_obj.obj").c_str(),
		(TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str(),
		(SHADERS_DIR  + "V_trianglePCT.spv").c_str(),
		(SHADERS_DIR  + "f_trianglePCT.spv").c_str(),
		VertexType(sizeof(VertexPCT), 1, 1, 1),
		primitiveTopology::triangle);

	// Delete a model you passed previously.
	app.deleteModel(assets["cottage"]);

	// Add the same model again.
	assets["cottage"] = app.newModel( 1,
		MVP,
		(MODELS_DIR + "cottage_obj.obj").c_str(),
		(TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str(),
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		VertexType(sizeof(VertexPCT), 1, 1, 1),
		primitiveTopology::triangle);
}

void setRoom(Renderer& app)
{
	assets["room"] = app.newModel( 2,
		MVP,
		(MODELS_DIR + "viking_room.obj").c_str(),
		(TEXTURES_DIR + "viking_room.png").c_str(),
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		VertexType(sizeof(VertexPCT), 1, 1, 1),
		primitiveTopology::triangle);

	assets["room"]->setUBO(0, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, -90.0f), glm::vec3(0.0f, -50.0f, 3.0f)));
	assets["room"]->setUBO(1, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -80.0f, 3.0f)));
	//assets["room"]->setUBO(2, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f,  90.0f), glm::vec3(30.0f, -80.0f, 3.0f)));
	//assets["room"]->setUBO(3, modelMatrix(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 180.0f), glm::vec3(30.0f, -50.0f, 3.0f)));
}

void setFloor(Renderer& app)
{
	assets["floor"] = app.newModel( 1,
		MVP,
		VertexType(sizeof(VertexPT), 1, 0, 1), 4, v_floor.data(),
		&i_floor,
		(TEXTURES_DIR + "grass.png").c_str(),
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		primitiveTopology::triangle );

	//assets["floor"]->setUBO(0, modelMatrix());
}

void setTerrain(Renderer& app)
{
	terrGen.computeTerrain(noiser, 0, 0, 5, 20, 20, 1.f);

	assets["terrain"] = app.newModel( 1,
		MVP,
		VertexType(PTN, 1, 0, 1, 1), terrGen.getNumVertex(), terrGen.vertex,
		&terrGen.indices,
		(TEXTURES_DIR + "squares.png").c_str(),
		(SHADERS_DIR + "v_terrainPTN.spv").c_str(),
		(SHADERS_DIR + "f_terrainPTN.spv").c_str(),
		primitiveTopology::triangle);
}