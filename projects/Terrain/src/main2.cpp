#include <filesystem>
#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "toolkit.hpp"
#include "ECSarch.hpp"

#include "terrain.hpp"
#include "common.hpp"
#include "entities.hpp"
#include "components.hpp"
#include "systems.hpp"

//#define DEBUG_MAIN 

// Prototypes
void update(Renderer& rend, glm::mat4 view, glm::mat4 proj);
void setLights();
float getFloorHeight(const glm::vec3& pos);
float getSeaHeight(const glm::vec3& pos);

void setSkybox(Renderer& app);			// L0, T (layer 0, Triangles)
void setSun(Renderer& app);				// L0, T
void setPoints(Renderer& app);			// L1, P
void setAxis(Renderer& app);			// L1, L
void setGrid(Renderer& app);			// L1, L
void setCottage(Renderer& app);			// L1, M
void setRoom(Renderer& app);			// L1, M
void setChunk(Renderer& app);			// L1, T
void setChunkGrid(Renderer& app);		// L1, T
void setAtmosphere(Renderer& app);		// --, PP
void setReticule(Renderer& app);		// --, PP
void setNoPP(Renderer& app);			// --, PP
// <<< Add 3D grid
void tests();

// Models, textures, & shaders
Renderer app(update, 2);					// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
std::map<std::string, modelIter> assets;	// Model iterators

// Others
int gridStep = 50;
ifOnce check;									// LOOK implement as functor (function with state)
LightSet lights(3);
SunSystem sun(0.00, 0.0, 3.14/10, 500.f, 2);	// Params: dayTime, speed, angularWidth, distance, mode

SingleNoise noiser_1(	// Desert
	FastNoiseLite::NoiseType_Cellular,	// Noise type
	4, 1.5, 0.28f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
	1, 70,								// Scale, Multiplier
	0,									// Curve degree
	500, 500, 0,						// XYZ offsets
	4952);								// Seed

SingleNoise noiser_2(	// Hills
	FastNoiseLite::NoiseType_Perlin,	// Noise type
	3, 6.f, 0.1f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
	3, 120,								// Scale, Multiplier
	1,									// Curve degree
	0, 0, 0,							// XYZ offsets
	4952);								// Seed

PlainChunk singleChunk(app, &noiser_1, glm::vec3(100, 29, 0), 5, 41, 11);
TerrainGrid terrGrid(&app, &noiser_1, lights, 6400, 29, 8, 2, 1.2, false);

Planet planetGrid   (&app, &noiser_2, lights, 100, 29, 8, 2, 1.2f, 2000, { 0.f, 0.f, 0.f }, false);
Sphere planetSeaGrid(&app, lights, 100, 29, 8, 2, 1.f, 2010, { 0.f, 0.f, 0.f }, true);

GrassSystem_planet grass(app, lights, planetGrid, 20, 6);

bool updateChunk = false, updateChunkGrid = false;

dataForUpdates d;

EntityManager world;

// main ---------------------------------------------------------------------

int main(int argc, char* argv[])
{
	#ifdef DEBUG_MAIN
		//std::cout << std::setprecision(7);
		std::cout << "Current path: " << std::filesystem::current_path() << std::endl;
		std::cout << "PlanetGrid area: " << planetGrid.getSphereArea() << std::endl;
		TimerSet time;
		std::cout << "------------------------------" << std::endl << time.getDate() << std::endl;
	#endif

	//tests(); return 0;

	try   // https://www.tutorialspoint.com/cplusplus/cpp_exceptions_handling.htm
	{
		EntityFactory eFact(app);
		
		world.addEntity(std::vector<Component*>{ new c_Engine(app), new c_Input, new c_Camera(1) });	// Singleton components.
		world.addEntity(eFact.createSkyBox(ShaderLoaders[4], ShaderLoaders[5], { texInfos[0] }));
		world.addEntity(eFact.createAxes(ShaderLoaders[2], ShaderLoaders[3], { }));
		world.addEntity(eFact.createNoPP(ShaderLoaders[22], ShaderLoaders[23], { texInfos[4], texInfos[5] }));
		
		world.addSystem(new s_Engine);
		world.addSystem(new s_Input);
		world.addSystem(new s_SphereCam);	// s_SphereCam, s_PolarCam, s_PlaneCam, s_FPCam
		world.addSystem(new s_Position);
		world.addSystem(new s_ModelMatrix);
		world.addSystem(new s_UBO);

		//world.printInfo();
		//return 0;

		//camera_4.camParticle.setCallback(getFloorHeight);

		//setLights();

		//---setPoints(app);
		//--setAxis(app);
		//---setGrid(app);
		//--setSkybox(app);
		//setCottage(app);
		//setRoom(app);
		//setChunk(app);
		//setChunkGrid(app);
		 //planetGrid.addResources(std::vector<ShaderLoader>{ShaderLoaders[14], ShaderLoaders[15]}, usedTextures);
		 //planetSeaGrid.addResources(std::vector<ShaderLoader>{ShaderLoaders[10], ShaderLoaders[11]}, usedTextures);
		 //grass.createGrassModel(std::vector<ShaderLoader>{ShaderLoaders[8], ShaderLoaders[9]}, std::vector<TextureLoader>{ texInfos[37], texInfos[38], texInfos[39], texInfos[40] });

		//--setNoPP(app);			// In the same layer, the last drawn 
		//setAtmosphere(app);	// Draw atmosphere first and reticule second, so atmosphere isn't hidden by reticule's transparent pixels

		//setSun(app);
		//setReticule(app);

		app.renderLoop();		// Start rendering
		if (0) throw "Test exception";
	}
	catch (std::exception e) { std::cout << e.what() << std::endl; }
	catch (const char* msg) { std::cout << msg << std::endl; }

	#ifdef DEBUG_MAIN
		std::cout << "main() end" << std::endl;
	#endif

	if (STANDALONE_EXECUTABLE) system("pause");
	return EXIT_SUCCESS;
}

/*
int main2(int argc, char* argv[])
{
	#ifdef DEBUG_MAIN
		//std::cout << std::setprecision(7);
		std::cout << "Current path: " << std::filesystem::current_path() << std::endl;
		std::cout << "PlanetGrid area: " << planetGrid.getSphereArea() << std::endl;
		TimerSet time;
		std::cout << "------------------------------" << std::endl << time.getDate() << std::endl;
	#endif

	//tests(); return 0;

	try   // https://www.tutorialspoint.com/cplusplus/cpp_exceptions_handling.htm
	{
		camera_4.camParticle.setCallback(getFloorHeight);

		setLights();
		
		//setPoints(app);
		setAxis(app);
		//setGrid(app);
		setSkybox(app);
		//setCottage(app);
		//setRoom(app);
		//setChunk(app);
		//setChunkGrid(app);
		planetGrid.addResources(std::vector<ShaderLoader>{ShaderLoaders[14], ShaderLoaders[15]}, usedTextures);
		planetSeaGrid.addResources(std::vector<ShaderLoader>{ShaderLoaders[10], ShaderLoaders[11]}, usedTextures);
		grass.createGrassModel(std::vector<ShaderLoader>{ShaderLoaders[8], ShaderLoaders[9]}, std::vector<TextureLoader>{ texInfos[37], texInfos[38], texInfos[39], texInfos[40] });

		//setNoPP(app);			// In the same layer, the last drawn 
		setAtmosphere(app);		// Draw atmosphere first and reticule second, so atmosphere isn't hidden by reticule's transparent pixels

		setSun(app);
		//setReticule(app);
		
		app.renderLoop();		// Start rendering
		if (0) throw "Test exception";
	}
	catch (std::exception e) { std::cout << e.what() << std::endl; }
	catch (const char* msg) { std::cout << msg << std::endl; }

	#ifdef DEBUG_MAIN
		std::cout << "main() end" << std::endl;
	#endif
	
	if(STANDALONE_EXECUTABLE) system("pause");
	return EXIT_SUCCESS;
}
*/

void update(Renderer& rend, glm::mat4 view, glm::mat4 proj)
{
	d.frameTime = (float)(rend.getTimer().getTime());
	d.aspectRatio = rend.getAspectRatio();
	//d.fov = rend.getCamera().fov;
	//d.clipPlanes[0] = rend.getCamera().nearViewPlane;
	//d.clipPlanes[1] = rend.getCamera().farViewPlane;
	d.screenSize = rend.getScreenSize();
	d.fps = rend.getTimer().getFPS();
	d.maxfps = rend.getTimer().getMaxPossibleFPS();
	//d.camPos = rend.getCamera().camPos;
	//d.camDir = rend.getCamera().getFront();
	//d.camUp = rend.getCamera().getCamUp();
	//d.camRight = rend.getCamera().getRight();
	d.groundHeight = planetGrid.getGroundHeight(d.camPos);
	size_t i;

	world.update(rend.getTimer().getDeltaTime());

	#ifdef DEBUG_MAIN
		std::cout << rend.getFrameCount() << ") " << std::endl;
		//std::cout << rend.getFrameCount() << ") " << fps << '\n';
		//std::cout << ") \n  Commands: " << rend.getCommandsCount() / 3 << std::endl;
	#endif

	// Time
	sun.updateTime(d.frameTime);

	// Light
	lights.posDir[0].direction = sun.lightDirection();	// Directional (sun)

	lights.posDir[1].direction = -sun.lightDirection();	// Directional (night)

	lights.posDir[2].position  = d.camPos;
	lights.posDir[2].direction = d.camDir;

	//std::cout
	//	<< "camPos: " << pos.x << ", " << pos.y << ", " << pos.z << " | "
	//	<< "moveSpeed: " << rend.getCamera().moveSpeed << " | "
	//	<< "mouseSensitivity: " << rend.getCamera().mouseSensitivity << " | "
	//	<< "scrollSpeed: " << rend.getCamera().scrollSpeed << " | "
	//	<< "fov: " << rend.getCamera().fov << " | "
	//	<< "YPR: " << rend.getCamera().yaw << ", " << rend.getCamera().pitch << ", " << rend.getCamera().roll << " | "
	//	<< "N/F planes: " << rend.getCamera().nearViewPlane << ", " << rend.getCamera().farViewPlane << " | "
	//	<< "yScrollOffset: " << rend.getCamera().yScrollOffset << " | "
	//	<< "worldUp: " << rend.getCamera().worldUp.x << ", " << rend.ge65Camera().worldUp.y << ", " << rend.getCamera().worldUp.z << std::endl;

	// Chunks
	if (updateChunk) singleChunk.updateUBOs(view, proj, d.camPos, lights, d.frameTime, d.groundHeight);

	if(updateChunkGrid)
	{
		//std::cout << "  Nodes: " << terrGrid.getRenderedChunks() << '/' << terrGrid.getloadedChunks() << '/' << terrGrid.getTotalNodes() << std::endl;
		terrGrid.updateTree(d.camPos);
		terrGrid.updateUBOs(view, proj, d.camPos, lights, d.frameTime, d.groundHeight);
	}
	
	planetGrid.updateState(d.camPos, view, proj, lights, d.frameTime, d.groundHeight);
	
	planetSeaGrid.updateState(d.camPos, view, proj, lights, d.frameTime, d.groundHeight);
	planetSeaGrid.toLastDraw();

	grass.updateGrass(d.camPos, planetGrid, view, proj, d.frameTime, d.fov, d.camDir);
	grass.toLastDraw();

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
		memcpy(assets["points"]->vsDynUBO.getUBOptr(0), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, -90.f), glm::vec3( 0.f, -50.f, 3.f)), mat4size);
		memcpy(assets["points"]->vsDynUBO.getUBOptr(1), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f,   0.f), glm::vec3( 0.f, -80.f, 3.f)), mat4size);
		memcpy(assets["points"]->vsDynUBO.getUBOptr(2), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f,  90.f), glm::vec3(30.f, -80.f, 3.f)), mat4size);
		memcpy(assets["points"]->vsDynUBO.getUBOptr(3), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, 180.f), glm::vec3(30.f, -50.f, 3.f)), mat4size);
	}
*/

// Update UBOs
	uint8_t* dest;

	if (assets.find("reticule") != assets.end())
		for (i = 0; i < assets["reticule"]->vsDynUBO.numDynUBOs; i++)
			memcpy(assets["reticule"]->vsDynUBO.getUBOptr(i), &d.aspectRatio, sizeof(d.aspectRatio));

	if (assets.find("atmosphere") != assets.end())
		for (i = 0; i < assets["atmosphere"]->vsDynUBO.numDynUBOs; i++)
		{
			dest = assets["atmosphere"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * size.vec4, &d.fov, sizeof(d.fov));
			memcpy(dest + 1 * size.vec4, &d.aspectRatio, sizeof(d.aspectRatio));
			memcpy(dest + 2 * size.vec4, &d.camPos, sizeof(d.camPos));
			memcpy(dest + 3 * size.vec4, &d.camDir, sizeof(d.camDir));
			memcpy(dest + 4 * size.vec4, &d.camUp, sizeof(d.camUp));
			memcpy(dest + 5 * size.vec4, &d.camRight, sizeof(d.camRight));
			memcpy(dest + 6 * size.vec4, &lights.posDir[0].direction, sizeof(glm::vec3));
			memcpy(dest + 7 * size.vec4, &d.clipPlanes, sizeof(glm::vec2));
			memcpy(dest + 8 * size.vec4, &d.screenSize, sizeof(glm::vec2));
			memcpy(dest + 9 * size.vec4, &view, sizeof(view));
			memcpy(dest + 9 * size.vec4 + 1 * size.mat4, &proj, sizeof(proj));
		}

	if (assets.find("points") != assets.end())
		for (i = 0; i < assets["points"]->vsDynUBO.numDynUBOs; i++)	{
			dest = assets["points"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
		}

	if (assets.find("axis") != assets.end())
		for (i = 0; i < assets["axis"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["axis"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
		}

	if (assets.find("grid") != assets.end())
		for (i = 0; i < assets["grid"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["grid"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * size.mat4, &modelMatrix(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(gridStep * ((int)d.camPos.x / gridStep), gridStep * ((int)d.camPos.y / gridStep), 0.0f)), size.mat4);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
		}

	if (0)//(assets.find("skyBox") != assets.end())
		for (i = 0; i < assets["skyBox"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["skyBox"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * size.mat4, &modelMatrix(glm::vec3(600.f, 600.f, 600.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(d.camPos.x, d.camPos.y, d.camPos.z)), size.mat4);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
		}

	if (assets.find("cottage") != assets.end())
		for (i = 0; i < assets["cottage"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["cottage"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * size.mat4, &modelMatrix(glm::vec3(1.f, 1.f, 1.f), glm::vec3(pi/2, d.frameTime * pi/2, 0.f), glm::vec3(0.f, 0.f, 0.f)), size.mat4);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
		}
		
	if (assets.find("room") != assets.end())
		for (i = 0; i < assets["room"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["room"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
		}

	if (assets.find("sun") != assets.end())
		for (i = 0; i < assets["sun"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["sun"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * size.mat4, &sun.MM(d.camPos), size.mat4);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
		}
	
	if (assets.find("sea") != assets.end())
	{
		app.toLastDraw(assets["sea"]);
		for (i = 0; i < assets["sea"]->vsDynUBO.numDynUBOs; i++) 
		{
			dest = assets["sea"]->vsDynUBO.getUBOptr(0);
			memcpy(dest + 0 * size.mat4, &modelMatrix(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(d.camPos.x, d.camPos.y, 0)), size.mat4);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
			memcpy(dest + 3 * size.mat4, &modelMatrixForNormals(modelMatrix()), size.mat4);
			memcpy(dest + 4 * size.mat4, &d.camPos, size.vec3);
			memcpy(dest + 4 * size.mat4 + size.vec4, lights.posDir, lights.posDirBytes);

			dest = assets["sea"]->fsUBO.getUBOptr(0);					// << Add to dest when advancing pointer
			memcpy(dest + 0 * size.vec4, &d.frameTime, sizeof(d.frameTime));
			memcpy(dest + 1 * size.vec4, lights.props, lights.propsBytes);
		}
	}
}


void setLights()
{
	#ifdef DEBUG_MAIN
		std::cout << "> " << __func__ << "()" << std::endl;
	#endif

	//lightss.turnOff(0);
	lights.setDirectional(0,  sun.lightDirection(), glm::vec3(0.03, 0.03, 0.03), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));						// Sun
	lights.setDirectional(1, -sun.lightDirection(), glm::vec3(0.00, 0.00, 0.00), glm::vec3(0.01, 0.01, 0.01), glm::vec3(0.007, 0.007, 0.007));	// Night
	lights.setSpot(2, glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), glm::vec3(2, 2, 2), glm::vec3(2, 2, 2), 1, 0.09, 0.032, 0.9, 0.8);
	//lights.setPoint(1, glm::vec3(0,0,0), glm::vec3(0,0,0), glm::vec3(40, 40, 40), glm::vec3(40, 40, 40), 1, 1, 1);
	//lights.setSpot(1, glm::vec3(0,0,0), glm::vec3(0, 0,-1), glm::vec3(0, 0, 0), glm::vec3(0, 40, 40), glm::vec3(40, 40, 40), 1, 1, 1, 0.9, 0.8);
}

float getFloorHeight(const glm::vec3& pos)
{
	glm::vec3 espheroid = glm::normalize(pos - planetGrid.nucleus) * planetGrid.radius;
	return 1.70 + planetGrid.radius + noiser_2.GetNoise(espheroid.x, espheroid.y, espheroid.z);
}

float getSeaHeight(const glm::vec3& pos)
{
	return 1.70 + planetSeaGrid.radius;
}



void setCottage(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	VerticesLoader vertexData(vt_332.vertexSize, vertexDir + "cottage_obj.obj");
	std::vector<ShaderLoader> shaders{ ShaderLoaders[6], ShaderLoaders[7] };
	std::vector<TextureLoader> textures{ texInfos[1] };

	assets["cottage"] = app.newModel(
		"cottage",
		1, 1, primitiveTopology::triangle, vt_332,
		vertexData, shaders, textures,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);
}

void setRoom(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	VerticesLoader vertexData(vt_332.vertexSize, vertexDir + "viking_room.obj");
	std::vector<ShaderLoader> shaders{ ShaderLoaders[6], ShaderLoaders[7] };
	std::vector<TextureLoader> textures{ texInfos[2] };

	assets["room"] = app.newModel(
		"room",
		1, 1, primitiveTopology::triangle, vt_332,
		vertexData, shaders, textures,
		2, 3 * size.mat4,	// M, V, P
		0,
		false);

	app.setRenders(assets["room"], 2);	// TEST (before render loop): setRenders
	app.setRenders(assets["room"], 3);	// TEST(in render loop) : setRenders(out of range)

	memcpy(assets["room"]->vsDynUBO.getUBOptr(0), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, -pi / 2), glm::vec3(2030.f, 0.f, 0.f)), size.mat4);
	memcpy(assets["room"]->vsDynUBO.getUBOptr(1), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 2030.f, 0.f)), size.mat4);
	memcpy(assets["room"]->vsDynUBO.getUBOptr(2), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, pi / 2), glm::vec3(0.f, 0.f, 2030.f)), size.mat4);
	//memcpy(assets["room"]->vsDynUBO.getUBOptr(3), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f,  pi), glm::vec3(30.f, -50.f, 3.f)), size.mat4);
}

void setChunk(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	updateChunk = true;
	std::vector<ShaderLoader> shaders{ ShaderLoaders[12], ShaderLoaders[13] };

	singleChunk.computeTerrain(true);
	singleChunk.render(shaders, usedTextures, nullptr, lights.numLights, false);
}

void setChunkGrid(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	updateChunkGrid = true;

	std::vector<ShaderLoader> shaders{ ShaderLoaders[12], ShaderLoaders[13] };

	terrGrid.addResources(shaders, usedTextures);
	//terrChunks.updateTree(glm::vec3(0,0,0));
}

void setSun(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	std::vector<float> v_sun;	// [4 * 5]
	std::vector<uint16_t> i_sun;
	size_t numVertex = getQuad(v_sun, i_sun, 1.f, 1.f, 0.f);		// LOOK dynamic adjustment of reticule size when window is resized

	VerticesLoader vertexData(vt_32.vertexSize, v_sun.data(), numVertex, i_sun);
	std::vector<ShaderLoader> shaders{ ShaderLoaders[16], ShaderLoaders[17] };
	std::vector<TextureLoader> textures{ texInfos[4] };

	assets["sun"] = app.newModel(
		"sun",
		0, 1, primitiveTopology::triangle, vt_32,
		vertexData, shaders, textures,
		1, 3 * size.mat4,	// M, V, P
		0,
		true);
}

void setReticule(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	std::vector<float> v_ret;	// [4 * 5] ;
	std::vector<uint16_t> i_ret;
	getScreenQuad(v_ret, i_ret, 0.1, 0.1);

	VerticesLoader vertexData(vt_32.vertexSize, v_ret.data(), 4, i_ret);
	std::vector<ShaderLoader> shaders{ ShaderLoaders[18], ShaderLoaders[19] };
	std::vector<TextureLoader> textures{ texInfos[5] };

	assets["reticule"] = app.newModel(
		"reticule",
		2, 1, primitiveTopology::triangle, vt_32,
		vertexData, shaders, textures,
		1, size.vec4,				// aspect ratio (float)
		0,
		true);
}

void setAtmosphere(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	std::vector<float> v_quad;	// [4 * 5]
	std::vector<uint16_t> i_quad;
	getScreenQuad(v_quad, i_quad, 1.f, 0.5);

	OpticalDepthTable optDepth(10, 1400, 2450, 30, pi / 20, 10);	// numOptDepthPoints, planetRadius, atmosphereRadius, heightStep, angleStep, densityFallOff
	TextureLoader texOD(optDepth.table.data(), optDepth.angleSteps, optDepth.heightSteps, "optDepth", VK_FORMAT_R32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	DensityVector density(1400, 2450, 30, 10);						// planetRadius, atmosphereRadius, heightStep, densityFallOff
	TextureLoader texDV(density.table.data(), 1, density.heightSteps, "density", VK_FORMAT_R32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	VerticesLoader vertexData(vt_32.vertexSize, v_quad.data(), 4, i_quad);
	std::vector<ShaderLoader> shaders{ ShaderLoaders[20], ShaderLoaders[21] };
	std::vector<TextureLoader> textures{ texOD, texDV };

	assets["atmosphere"] = app.newModel(
		"atmosphere",
		2, 1, primitiveTopology::triangle, vt_32,
		vertexData, shaders, textures,
		1, 2 * size.mat4 + 8 * size.vec4,
		0,
		false,
		1);
}

// ----------------------------------------------------------

void setNoPP(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	std::vector<float> v_quad;	// [4 * 5]
	std::vector<uint16_t> i_quad;
	getScreenQuad(v_quad, i_quad, 1.f, 0.5);	// <<< The parameter zValue doesn't represent heigth (otherwise, this value should serve for hiding one plane behind another).

	VerticesLoader vertexData(vt_32.vertexSize, v_quad.data(), 4, i_quad);
	std::vector<ShaderLoader> shaders{ ShaderLoaders[22], ShaderLoaders[23] };
	std::vector<TextureLoader> textures{ texInfos[4], texInfos[5] };

	assets["noPP"] = app.newModel(
		"noPP",
		2, 1, primitiveTopology::triangle, vt_32,		// For post-processing, we select an out-of-range layer so this model is not processed in the first pass (layers are only used in first pass).
		vertexData, shaders, textures,
		1, 1,											// <<< ModelSet doesn't work if there is no dynUBO_vs
		0,
		false,
		1);
}


void setPoints(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	Icosahedron icos(30.f);	// Just created for calling destructor, which applies a multiplier.

	VerticesLoader vertexData(vt_33.vertexSize, Icosahedron::icos.data(), Icosahedron::icos.size() / 6, noIndices);
	std::vector<ShaderLoader> shaders{ ShaderLoaders[0], ShaderLoaders[1] };

	assets["points"] = app.newModel(
		"points",
		1, 1, primitiveTopology::point, vt_33,
		vertexData, shaders, noTextures,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);

	memcpy(assets["points"]->vsDynUBO.getUBOptr(0), &modelMatrix(), size.mat4);
}

void setAxis(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	std::vector<float> v_axis;
	std::vector<uint16_t> i_axis;
	size_t numVertex = getAxis(v_axis, i_axis, 5000, 0.9);		// getAxis(), getLongAxis()

	VerticesLoader vertexData(vt_33.vertexSize, v_axis.data(), numVertex, i_axis);
	std::vector<ShaderLoader> shaders{ ShaderLoaders[2], ShaderLoaders[3] };

	assets["axis"] = app.newModel(
		"axis",
		1, 1, primitiveTopology::line, vt_33,
		vertexData, shaders, noTextures,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);

	memcpy(assets["axis"]->vsDynUBO.getUBOptr(0), &modelMatrix(), size.mat4);
}

void setGrid(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	std::vector<float> v_grid;
	std::vector<uint16_t> i_grid;
	size_t numVertex = getGrid(v_grid, i_grid, gridStep, 50, 100, glm::vec3(0.5, 0.5, 0.5));

	VerticesLoader vertexData(vt_33.vertexSize, v_grid.data(), numVertex, i_grid);
	std::vector<ShaderLoader> shaders{ ShaderLoaders[2], ShaderLoaders[3] };

	assets["grid"] = app.newModel(
		"grid",
		1, 1, primitiveTopology::line, vt_33,
		vertexData, shaders, noTextures,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);
}

void setSkybox(Renderer& app)
{
#ifdef DEBUG_MAIN
	std::cout << "> " << __func__ << "()" << std::endl;
#endif

	VerticesLoader vertexData(vt_32.vertexSize, v_cube.data(), 14, i_inCube);
	std::vector<ShaderLoader> shaders{ ShaderLoaders[4], ShaderLoaders[5] };
	std::vector<TextureLoader> textures{ texInfos[0] };

	assets["skyBox"] = app.newModel(
		"skyBox",
		0, 1, primitiveTopology::triangle, vt_32,
		vertexData, shaders, textures,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);
}


void tests()
{

}