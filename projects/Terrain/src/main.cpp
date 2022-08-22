/*
	Renderer	< VulkanEnvironment
				< ModelData		< VulkanEnvironment
								< modelConfig	< VulkanEnvironment
												< getModelMatrix callbacks
								< Input			< Camera
*/

#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "toolkit.hpp"

#include "terrain.hpp"
#include "common.hpp"


// Prototypes
void update(Renderer& rend, glm::mat4 view, glm::mat4 proj);
void loadTextures(Renderer& app);
void setLights();

void setPoints(Renderer& app);
void setAxis(Renderer& app);
void setGrid(Renderer& app);
void setSkybox(Renderer& app);
void setCottage(Renderer& app);
void setRoom(Renderer& app);
void setChunk(Renderer& app);
void setChunkSet(Renderer& app);
void setSphereChunks(Renderer& app);
void setSphereSet(Renderer& app);
void setSun(Renderer& app);
void setReticule(Renderer& app);

// Models & textures
Renderer app(update, &camera_3, 3);				// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
std::map<std::string, modelIterator> assets;	// Model iterators
std::map<std::string, texIterator> textures;	// Texture iterators

// Others
int gridStep = 50;
ifOnce check;			// LOOK implement as functor (function with state)
std::vector<Light*> lights = { &sunLight };

// Terrain
Noiser noiser(
	FastNoiseLite::NoiseType_Cellular,	// Noise type
	4, 1.5, 0.28f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
	1, 70,								// Scale, Multiplier
	0,									// Curve degree
	500, 500, 0,						// XYZ offsets
	4952);								// Seed

PlainChunk singleChunk(app, noiser, std::tuple<float, float, float>(100, 25, 0), 5, 41, 11, lights);

TerrainGrid terrGrid(app, noiser, lights, 6400, 21, 8, 2, 1.2);

SphericalChunk sphereChunk_pX(app, noiser, std::tuple<float, float, float>( 50,  0,  0), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 1, 0, 0));
SphericalChunk sphereChunk_nX(app, noiser, std::tuple<float, float, float>(-50,  0,  0), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3(-1, 0, 0));
SphericalChunk sphereChunk_pY(app, noiser, std::tuple<float, float, float>(  0, 50,  0), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 1, 0));
SphericalChunk sphereChunk_nY(app, noiser, std::tuple<float, float, float>(  0,-50,  0), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 0,-1, 0));
SphericalChunk sphereChunk_pZ(app, noiser, std::tuple<float, float, float>(  0,  0, 50), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 0, 1));
SphericalChunk sphereChunk_nZ(app, noiser, std::tuple<float, float, float>(  0,  0,-50), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 0,-1));

PlanetGrid planetGrid_pZ(app, noiser, lights, 100, 21, 8, 2, 1.2f, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 0, 1), glm::vec3(  0,  0, 50));
PlanetGrid planetGrid_nZ(app, noiser, lights, 100, 21, 8, 2, 1.2f, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 0,-1), glm::vec3(  0,  0,-50));
PlanetGrid planetGrid_pY(app, noiser, lights, 100, 21, 8, 2, 1.2f, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 1, 0), glm::vec3(  0, 50,  0));
PlanetGrid planetGrid_nY(app, noiser, lights, 100, 21, 8, 2, 1.2f, 1000, glm::vec3(0, 0, 0), glm::vec3( 0,-1, 0), glm::vec3(  0,-50,  0));
PlanetGrid planetGrid_pX(app, noiser, lights, 100, 21, 8, 2, 1.2f, 1000, glm::vec3(0, 0, 0), glm::vec3( 1, 0, 0), glm::vec3( 50,  0,  0));
PlanetGrid planetGrid_nX(app, noiser, lights, 100, 21, 8, 2, 1.2f, 1000, glm::vec3(0, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(-50,  0,  0));

bool updateChunk = false, updateChunkSet = false, updatePlanet = false, updatePlanetSet = false;

// Data to update
long double frameTime;
size_t fps;
size_t maxfps;
glm::vec3 pos;


int main(int argc, char* argv[])
{
	TimerSet time;
	std::cout << "------------------------------" << std::endl << time.getDate() << std::endl;

	setLights();
	loadTextures(app);

	//setPoints(app);
	setAxis(app);
	//setGrid(app);
	setSkybox(app);
	//setCottage(app);
	//setRoom(app);
	//setChunk(app);
	//setChunkSet(app);
	//setSphereChunks(app);
	setSphereSet(app);
	setSun(app);
	setReticule(app);

	app.run();		// Start rendering

	std::cout << "main() end" << std::endl;
	return EXIT_SUCCESS;
}


void update(Renderer& rend, glm::mat4 view, glm::mat4 proj)
{
	frameTime	= rend.getTimer().getTime();
	fps			= rend.getTimer().getFPS();
	maxfps		= rend.getTimer().getMaxPossibleFPS();
	pos			= rend.getCamera().camPos;
	size_t i;
	
	std::cout << rend.getFrameCount() << ')';
	//std::cout << ") \n  Commands: " << rend.getCommandsCount() / 3 << std::endl;

	//std::cout
	//	<< "camPos: " << pos.x << ", " << pos.y << ", " << pos.z << " | "
	//	<< "moveSpeed: " << rend.getCamera().moveSpeed << " | "
	//	<< "mouseSensitivity: " << rend.getCamera().mouseSensitivity << " | "
	//	<< "scrollSpeed: " << rend.getCamera().scrollSpeed << " | "
	//	<< "fov: " << rend.getCamera().fov << " | "
	//	<< "YPR: " << rend.getCamera().yaw << ", " << rend.getCamera().pitch << ", " << rend.getCamera().roll << " | "
	//	<< "N/F planes: " << rend.getCamera().nearViewPlane << ", " << rend.getCamera().farViewPlane << " | "
	//	<< "yScrollOffset: " << rend.getCamera().yScrollOffset << " | "
	//	<< "worldUp: " << rend.getCamera().worldUp.x << ", " << rend.getCamera().worldUp.y << ", " << rend.getCamera().worldUp.z << std::endl;

	// Chunks
	if (updateChunk) singleChunk.updateUBOs(pos, view, proj);

	if(updateChunkSet)
	{
		std::cout << "  Nodes: " << terrGrid.getloadedChunks() << '/' << terrGrid.getRenderedChunks() << '/' << terrGrid.getTotalNodes() << std::endl;
		terrGrid.updateTree(pos);
		terrGrid.updateUBOs(pos, view, proj);
	}

	if (updatePlanet)
	{
		sphereChunk_pX.updateUBOs(pos, view, proj);
		sphereChunk_nX.updateUBOs(pos, view, proj);
		sphereChunk_pY.updateUBOs(pos, view, proj);
		sphereChunk_nY.updateUBOs(pos, view, proj);
		sphereChunk_pZ.updateUBOs(pos, view, proj);
		sphereChunk_nZ.updateUBOs(pos, view, proj);
	}

	if (updatePlanetSet)
	{
		//std::cout << "  Nodes: " << planetGrid_pZ.getloadedChunks() << '/' << planetGrid_pZ.getRenderedChunks() << '/' << planetGrid_pZ.getTotalNodes() << std::endl;
		planetGrid_pZ.updateTree(pos);
		planetGrid_pZ.updateUBOs(pos, view, proj);
		planetGrid_nZ.updateTree(pos);
		planetGrid_nZ.updateUBOs(pos, view, proj);
		planetGrid_pY.updateTree(pos);
		planetGrid_pY.updateUBOs(pos, view, proj);
		planetGrid_nY.updateTree(pos);
		planetGrid_nY.updateUBOs(pos, view, proj);
		planetGrid_pX.updateTree(pos);
		planetGrid_pX.updateUBOs(pos, view, proj);
		planetGrid_nX.updateTree(pos);
		planetGrid_nX.updateUBOs(pos, view, proj);
	}

/*
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
*/
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

	if (assets.find("sun") != assets.end())
		for (i = 0; i < assets["sun"]->vsDynUBO.dynBlocksCount; i++) {
			assets["sun"]->vsDynUBO.setUniform(i, 0, sunMM(pos, dayTime, 0.5f, sunAngDist));
			assets["sun"]->vsDynUBO.setUniform(i, 1, view);
			assets["sun"]->vsDynUBO.setUniform(i, 2, proj);
		}
}

void setLights()
{
	//sunLight.turnOff();
	sunLight.setDirectional(-sunLightDirection(dayTime), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));
	//sunLight.setPoint(glm::vec3(0, 0, 50), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 0.1, 0.01);
	//sunLight.setSpot(glm::vec3(0, 0, 150), glm::vec3(0, 0, 1), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 0, 0., 0.9, 0.8);
}

void loadTextures(Renderer& app)
{
	textures["skybox"] = app.newTexture((TEXTURES_DIR + "sky_box/space1.jpg").c_str());
	textures["cottage"] = app.newTexture((TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str());
	textures["room"] = app.newTexture((TEXTURES_DIR + "viking_room.png").c_str());
	textures["squares"] = app.newTexture((TEXTURES_DIR + "squares.png").c_str());
	textures["sun"] = app.newTexture((TEXTURES_DIR + "Sun/sun2_1.png").c_str());
	textures["reticule"] = app.newTexture((TEXTURES_DIR + "HUD/reticule_1.png").c_str());
	app.deleteTexture(textures["skybox"]);												// TEST (before render loop): deleteTexture
	textures["skybox"] = app.newTexture((TEXTURES_DIR + "sky_box/space1.jpg").c_str());	// TEST (before render loop): newTexture

	textures["grass"] = app.newTexture((TEXTURES_DIR + "grass.png").c_str());
	textures["grassSpec"] = app.newTexture((TEXTURES_DIR + "grass_specular.png").c_str());
	textures["rock"] = app.newTexture((TEXTURES_DIR + "rock.jpg").c_str());
	textures["rockSpec"] = app.newTexture((TEXTURES_DIR + "rock_specular.jpg").c_str());
	textures["sand"] = app.newTexture((TEXTURES_DIR + "sand.jpg").c_str());
	textures["sandSpec"] = app.newTexture((TEXTURES_DIR + "sand_specular.jpg").c_str());
	textures["plainSand"] = app.newTexture((TEXTURES_DIR + "plainSand.jpg").c_str());
	textures["plainSandSpec"] = app.newTexture((TEXTURES_DIR + "plainSand_specular.jpg").c_str());

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
	std::vector<uint16_t> i_axis;
	size_t numVertex = getAxis(v_axis, i_axis, 1000, 0.8);

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
	std::vector<uint16_t> i_grid;
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
	updateChunk = true;

	std::vector<texIterator> usedTextures = { textures["squares"], textures["grass"], textures["grassSpec"], textures["rock"], textures["rockSpec"], textures["sand"], textures["sandSpec"], textures["plainSand"], textures["plainSandSpec"] };

	singleChunk.computeTerrain(true);
	singleChunk.render((SHADERS_DIR + "v_terrainPTN.spv").c_str(), (SHADERS_DIR + "f_terrainPTN.spv").c_str(), usedTextures, nullptr);
}

void setChunkSet(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	updateChunkSet = true;

	std::vector<texIterator> usedTextures = { textures["squares"], textures["grass"], textures["grassSpec"], textures["rock"], textures["rockSpec"], textures["sand"], textures["sandSpec"], textures["plainSand"], textures["plainSandSpec"] };

	terrGrid.addTextures(usedTextures);
	terrGrid.addShaders((SHADERS_DIR + "v_terrainPTN.spv").c_str(), (SHADERS_DIR + "f_terrainPTN.spv").c_str());
	//terrChunks.updateTree(glm::vec3(0,0,0));
}

void setSphereChunks(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	updatePlanet = true;

	std::vector<texIterator> usedTextures = { textures["squares"], textures["grass"], textures["grassSpec"], textures["rock"], textures["rockSpec"], textures["sand"], textures["sandSpec"], textures["plainSand"], textures["plainSandSpec"] };

	sphereChunk_nY.computeTerrain(true);
	sphereChunk_nY.render((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str(), usedTextures, nullptr);
	
	sphereChunk_pX.computeTerrain(true);
	sphereChunk_pX.render((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str(), usedTextures, nullptr);
	
	sphereChunk_pZ.computeTerrain(true);
	sphereChunk_pZ.render((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str(), usedTextures, nullptr);
	
	sphereChunk_pY.computeTerrain(true);
	sphereChunk_pY.render((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str(), usedTextures, nullptr);
	
	sphereChunk_nX.computeTerrain(true);
	sphereChunk_nX.render((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str(), usedTextures, nullptr);
	
	sphereChunk_nZ.computeTerrain(true);
	sphereChunk_nZ.render((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str(), usedTextures, nullptr);
}

void setSphereSet(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	updatePlanetSet = true;

	std::vector<texIterator> usedTextures = { textures["squares"], textures["grass"], textures["grassSpec"], textures["rock"], textures["rockSpec"], textures["sand"], textures["sandSpec"], textures["plainSand"], textures["plainSandSpec"] };

	planetGrid_pZ.addTextures(usedTextures);
	planetGrid_pZ.addShaders((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str());

	planetGrid_nZ.addTextures(usedTextures);
	planetGrid_nZ.addShaders((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str());

	planetGrid_pY.addTextures(usedTextures);
	planetGrid_pY.addShaders((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str());

	planetGrid_nY.addTextures(usedTextures);
	planetGrid_nY.addShaders((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str());

	planetGrid_pX.addTextures(usedTextures);
	planetGrid_pX.addShaders((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str());

	planetGrid_nX.addTextures(usedTextures);
	planetGrid_nX.addShaders((SHADERS_DIR + "v_planetPTN.spv").c_str(), (SHADERS_DIR + "f_planetPTN.spv").c_str());

	//terrChunks.updateTree(glm::vec3(0,0,0));
}

void setSun(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<VertexPT> v_sun;
	std::vector<uint16_t> i_sun;
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
	std::vector<uint16_t> i_ret;
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