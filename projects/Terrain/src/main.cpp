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
void setLights();
void loadTextures(Renderer& app);
void loadShaders(Renderer& app);
float getFloorHeight(const glm::vec3& pos);

void setReticule(Renderer& app);
void setPoints(Renderer& app);
void setAxis(Renderer& app);
void setGrid(Renderer& app);
void setSea(Renderer& app);
void setSkybox(Renderer& app);
void setCottage(Renderer& app);
void setRoom(Renderer& app);
void setChunk(Renderer& app);
void setChunkGrid(Renderer& app);
void setSun(Renderer& app);

// Models, textures, & shaders
Renderer app(update, &camera_2, 3);				// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
std::map<std::string, modelIterator> assets;	// Model iterators
std::map<std::string, texIterator> textures;	// Texture iterators
std::map<std::string, ShaderIter> shaders;		// Shaders

// Others
int gridStep = 50;
ifOnce check;			// LOOK implement as functor (function with state)
//std::vector<Light*> lights = { &sunLight };
LightSet lightss(2u);
std::vector<texIterator> usedTextures;	// Package of textures from std::map<> textures

Noiser noiser_1(	// Desert
	FastNoiseLite::NoiseType_Cellular,	// Noise type
	4, 1.5, 0.28f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
	1, 70,								// Scale, Multiplier
	0,									// Curve degree
	500, 500, 0,						// XYZ offsets
	4952);								// Seed

Noiser noiser_2(	// Hills
	FastNoiseLite::NoiseType_Perlin,	// Noise type
	8, 8.f, 0.1f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
	3, 1,//120,								// Scale, Multiplier
	1,									// Curve degree
	0, 0, 0,							// XYZ offsets
	4952);								// Seed

PlainChunk singleChunk(app, noiser_1, glm::vec3(100, 25, 0), 5, 41, 11);
TerrainGrid terrGrid(app, noiser_1, lightss, 6400, 21, 8, 2, 1.2);

BasicPlanet planetChunks(app, noiser_2, 1, 101, 101, 1000, { 0.f, 0.f, 0.f });
Planet planetGrid(app, noiser_2, lightss, 100, 21, 8, 2, 1.2, 2000, { 0.f, 0.f, 0.f });

bool updateChunk = false, updateChunkGrid = false;

// Data to update
float frameTime;
size_t fps, maxfps;
glm::vec3 camPos, camDir;
float aspectRatio;


// main ---------------------------------------------------------------------

int main(int argc, char* argv[])
{
	std::cout << std::setprecision(7);
	camera_4.camParticle.setCallback(getFloorHeight);
	std::cout << "R: " << planetGrid.area << std::endl;
	
	TimerSet time;
	std::cout << "------------------------------" << std::endl << time.getDate() << std::endl;
	
	setLights();
	loadShaders(app);
	loadTextures(app);
	
	//setPoints(app);
	setAxis(app);
	//setGrid(app);
	//setSea(app);
	setSkybox(app);
	  //setCottage(app);
	  //setRoom(app);
	//setChunk(app);
	//setChunkGrid(app);
	//planetChunks.computeAndRender(usedTextures, shaders["v_planet"], shaders["f_planet"]);
	planetGrid.add_tex_shad(usedTextures, shaders["v_planet"], shaders["f_planet"]);
	setSun(app);
	setReticule(app);

	app.run();		// Start rendering

	std::cout << "main() end" << std::endl;
	return EXIT_SUCCESS;
}


void update(Renderer& rend, glm::mat4 view, glm::mat4 proj)
{
	//fps		= rend.getTimer().getFPS();
	//maxfps	= rend.getTimer().getMaxPossibleFPS();
	frameTime	= (float)rend.getTimer().getTime();
	camPos		= rend.getCamera().camPos;
	camDir		= rend.getCamera().getDirection();
	aspectRatio = rend.getAspectRatio();
	size_t i;
	
	std::cout << rend.getFrameCount() << ") \n";
	//std::cout << ") \n  Commands: " << rend.getCommandsCount() / 3 << std::endl;

	dayTime = 6.00;		// 0.00 + frameTime * 0.5;

	//sunLight.turnOff();
	sunLight.setDirectional  (Sun::lightDirection(dayTime), glm::vec3(0.03, 0.03, 0.03), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));
	//sunLight.setPoint(-10.f * Sun::lightDirection(dayTime), glm::vec3(0.03, 0.03, 0.03), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 1, 0);
	//sunLight.setSpot(-10.f * Sun::lightDirection(dayTime), glm::vec3(0, 1, 0), glm::vec3(0.03, 0.03, 0.03), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 1, 0., 0.9, 0.8);

	lightss.posDir[0].direction = Sun::lightDirection(dayTime);	// Directional (sun)
	lightss.posDir[1].position = camPos;
	lightss.posDir[1].direction = camDir;

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
	if (updateChunk) singleChunk.updateUBOs(view, proj, camPos, lightss, frameTime);

	if(updateChunkGrid)
	{
		//std::cout << "  Nodes: " << terrGrid.getRenderedChunks() << '/' << terrGrid.getloadedChunks() << '/' << terrGrid.getTotalNodes() << std::endl;
		terrGrid.updateTree(camPos);
		terrGrid.updateUBOs(view, proj, camPos, lightss, frameTime);
	}
		
	planetChunks.updateUbos(camPos, view, proj, lightss, frameTime);

	planetGrid.update_tree_ubo(camPos, view, proj, lightss, frameTime);

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
			memcpy(assets["reticule"]->vsDynUBO.getUBOptr(i), &aspectRatio, sizeof(aspectRatio));

	if (assets.find("points") != assets.end())
		for (i = 0; i < assets["points"]->vsDynUBO.numDynUBOs; i++)	{
			dest = assets["points"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
		}

	if (assets.find("axis") != assets.end())
		for (i = 0; i < assets["axis"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["axis"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
		}

	if (assets.find("grid") != assets.end())
		for (i = 0; i < assets["grid"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["grid"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * mat4size, &modelMatrix(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(gridStep * ((int)camPos.x / gridStep), gridStep * ((int)camPos.y / gridStep), 0.0f)), mat4size);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
		}

	if (assets.find("skyBox") != assets.end())
		for (i = 0; i < assets["skyBox"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["skyBox"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * mat4size, &modelMatrix(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(camPos.x, camPos.y, camPos.z)), mat4size);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
		}

	if (assets.find("cottage") != assets.end())
		for (i = 0; i < assets["cottage"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["cottage"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * mat4size, &modelMatrix(glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, frameTime * 45.f, 0.f), glm::vec3(0.f, 0.f, 0.f)), mat4size);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
		}
		
	if (assets.find("room") != assets.end())
		for (i = 0; i < assets["room"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["room"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
		}

	if (assets.find("sun") != assets.end())
		for (i = 0; i < assets["sun"]->vsDynUBO.numDynUBOs; i++) {
			dest = assets["sun"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 0 * mat4size, &Sun::MM(camPos, dayTime, 0.5f, sunAngDist), mat4size);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
		}
	
	if (assets.find("sea") != assets.end())
	{
		app.toLastDraw(assets["sea"]);
		for (i = 0; i < assets["sea"]->vsDynUBO.numDynUBOs; i++) 
		{
			dest = assets["sea"]->vsDynUBO.getUBOptr(0);
			memcpy(dest + 0 * mat4size, &modelMatrix(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(camPos.x, camPos.y, 0)), mat4size);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
			memcpy(dest + 3 * mat4size, &modelMatrixForNormals(modelMatrix()), mat4size);
			memcpy(dest + 4 * mat4size, &camPos, vec3size);
			memcpy(dest + 4 * mat4size + vec4size, lightss.posDir, lightss.posDirBytes);

			dest = assets["sea"]->fsUBO.getUBOptr(0);					// << Add to dest when advancing pointer
			memcpy(dest + 0 * vec4size, &frameTime, sizeof(frameTime));
			memcpy(dest + 1 * vec4size, lightss.props, lightss.propsBytes);
		}
	}
	
	//glm::vec3 viewDir = glm::normalize(camPos - glm::vec3(2000, 0, 0));
	//glm::vec3 reflectDir = glm::normalize(reflect(sunLight.direction, glm::vec3(0, 0, 1)));
	//std::cout << "> " << glm::dot(viewDir, reflectDir);

	//std::cout << '>' << sunLight.direction.z << " / " << std::endl;

	//glm::vec3 tangent = glm::normalize(vec3(ubo.model * glm::vec4(glm::cross(glm::vec3(0, 1, 0), inNormal), 0.f)));	// x
	//glm::vec3 bitangent = glm::normalize(glm::vec3(ubo.model * glm::vec4(cross(inNormal, tangent), 0.f)));	// y
	//glm::mat3 TBN = glm::transpose(glm::mat3(tangent, bitangent, inNormal));							// Transpose of an orthogonal matrix == its inverse (transpose is cheaper than inverse)
	//
	//outTangVertPos = TBN * vec3(ubo.model * vec4(inVertPos, 1.f));		// inverted TBN transforms vectors to tangent space
	//outTangCampPos = TBN * ubo.camPos.xyz;
	//outTangLightPos = TBN * ubo.lightPos.xyz;		// for point & spot light
	//outTangLightDir = TBN * ubo.lightDir.xyz;		// for directional light
}


void setLights()
{
	//lightss.turnOff(0);
	lightss.setDirectional(0, Sun::lightDirection(dayTime), glm::vec3(0.03, 0.03, 0.03), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));
	//lightss.setPoint(1, glm::vec3(0,0,0), glm::vec3(0,0,0), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 1, 0);
	lightss.setSpot(1, glm::vec3(0,0,0), glm::vec3(0, 0,-1), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 1, 0.5, 0.9, 0.8);
}

void loadShaders(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	shaders["v_point"] = app.newShader((shadersDir + "v_pointPC.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_point"] = app.newShader((shadersDir + "f_pointPC.frag").c_str(), shaderc_fragment_shader, true);

	shaders["v_line"] = app.newShader((shadersDir + "v_linePC.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_line"] = app.newShader((shadersDir + "f_linePC.frag").c_str(), shaderc_fragment_shader, true);

	shaders["v_skybox"] = app.newShader((shadersDir + "v_trianglePT.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_skybox"] = app.newShader((shadersDir + "f_trianglePT.frag").c_str(), shaderc_fragment_shader, true);

	shaders["v_house"] = app.newShader((shadersDir + "v_trianglePCT.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_house"] = app.newShader((shadersDir + "f_trianglePCT.frag").c_str(), shaderc_fragment_shader, true);

	shaders["v_sea"] = app.newShader((shadersDir + "v_sea.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_sea"] = app.newShader((shadersDir + "f_sea.frag").c_str(), shaderc_fragment_shader, true);

	shaders["v_terrain"] = app.newShader((shadersDir + "v_terrainPTN.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_terrain"] = app.newShader((shadersDir + "f_terrainPTN.frag").c_str(), shaderc_fragment_shader, true);

	shaders["v_planet"] = app.newShader((shadersDir + "v_planetPTN.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_planet"] = app.newShader((shadersDir + "f_planetPTN.frag").c_str(), shaderc_fragment_shader, true);

	shaders["v_sun"] = app.newShader((shadersDir + "v_sunPT.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_sun"] = app.newShader((shadersDir + "f_sunPT.frag").c_str(), shaderc_fragment_shader, true);

	shaders["v_hud"] = app.newShader((shadersDir + "v_hudPT.vert").c_str(), shaderc_vertex_shader, true);
	shaders["f_hud"] = app.newShader((shadersDir + "f_hudPT.frag").c_str(), shaderc_fragment_shader, true);
}

void loadTextures(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	// Special
	textures["skybox"]	 = app.newTexture((texDir + "sky_box/space1.jpg").c_str());
	textures["cottage"]  = app.newTexture((texDir + "cottage/cottage_diffuse.png").c_str());
	textures["room"]	 = app.newTexture((texDir + "viking_room.png").c_str());
	textures["squares"]  = app.newTexture((texDir + "squares.png").c_str());
	textures["sun"]		 = app.newTexture((texDir + "Sun/sun2_1.png").c_str());
	textures["reticule"] = app.newTexture((texDir + "HUD/reticule_1.png").c_str());
	//app.deleteTexture(textures["skybox"]);													// TEST (before render loop): deleteTexture
	//textures["skybox"]	 = app.newTexture((texDir + "sky_box/space1.jpg").c_str());	// TEST (before render loop): newTexture

	// Plants
	textures["grass1_a"] = app.newTexture((texDir + "grass1_a.png").c_str());
	textures["grass1_n"] = app.newTexture((texDir + "grass1_n.png").c_str());
	textures["grass1_s"] = app.newTexture((texDir + "grass1_s.png").c_str());
	textures["grass1_r"] = app.newTexture((texDir + "grass1_r.png").c_str());

	textures["grass2_a"] = app.newTexture((texDir + "grass2_a.png").c_str());
	textures["grass2_n"] = app.newTexture((texDir + "grass2_n.png").c_str());
	textures["grass2_s"] = app.newTexture((texDir + "grass2_s.png").c_str());
	textures["grass2_r"] = app.newTexture((texDir + "grass2_r.png").c_str());

	// Rocks
	textures["bumpRock_a"] = app.newTexture((texDir + "bumpRock_a.png").c_str());
	textures["bumpRock_n"] = app.newTexture((texDir + "bumpRock_n.png").c_str());
	textures["bumpRock_s"] = app.newTexture((texDir + "bumpRock_s.png").c_str());
	textures["bumpRock_r"] = app.newTexture((texDir + "bumpRock_r.png").c_str());

	textures["dryRock_a"]  = app.newTexture((texDir + "dryRock_a.png").c_str());
	textures["dryRock_n"]  = app.newTexture((texDir + "dryRock_n.png").c_str());
	textures["dryRock_s"]  = app.newTexture((texDir + "dryRock_s.png").c_str());
	textures["dryRock_r"]  = app.newTexture((texDir + "dryRock_r.png").c_str());

	// Soils
	textures["dunes_a"]  = app.newTexture((texDir + "dunes_a.png").c_str());
	textures["dunes_n"]  = app.newTexture((texDir + "dunes_n.png").c_str());
	textures["dunes_s"]  = app.newTexture((texDir + "dunes_s.png").c_str());
	textures["dunes_r"]  = app.newTexture((texDir + "dunes_r.png").c_str());

	textures["sand_a"]   = app.newTexture((texDir + "sand_a.png").c_str());
	textures["sand_n"]   = app.newTexture((texDir + "sand_n.png").c_str());
	textures["sand_s"]   = app.newTexture((texDir + "sand_s.png").c_str());
	textures["sand_r"]   = app.newTexture((texDir + "sand_r.png").c_str());

	textures["cobble_a"] = app.newTexture((texDir + "cobble_a.png").c_str());
	textures["cobble_n"] = app.newTexture((texDir + "cobble_n.png").c_str());
	textures["cobble_s"] = app.newTexture((texDir + "cobble_s.png").c_str());
	textures["cobble_r"] = app.newTexture((texDir + "cobble_r.png").c_str());

	// Water
	textures["sea_n"]   = app.newTexture((texDir + "sea_n.png").c_str());
					    
	textures["snow_a"]  = app.newTexture((texDir + "snow_a.png").c_str());
	textures["snow_n"]  = app.newTexture((texDir + "snow_n.png").c_str());
	textures["snow_s"]  = app.newTexture((texDir + "snow_s.png").c_str());
	textures["snow_r"]  = app.newTexture((texDir + "snow_r.png").c_str());

	textures["snow2_a"] = app.newTexture((texDir + "snow2_a.png").c_str());
	textures["snow2_n"] = app.newTexture((texDir + "snow2_n.png").c_str());
	textures["snow2_s"] = app.newTexture((texDir + "snow2_s.png").c_str());

	// Others
	textures["tech_a"] = app.newTexture((texDir + "tech_a.png").c_str());
	textures["tech_n"] = app.newTexture((texDir + "tech_n.png").c_str());
	textures["tech_s"] = app.newTexture((texDir + "tech_s.png").c_str());
	textures["tech_r"] = app.newTexture((texDir + "tech_r.png").c_str());

	// Package textures
	usedTextures =
	{
		/* 0 */  textures["squares"],

		/*1 - 4*/textures["grass1_a"], textures["grass1_n"], textures["grass1_s"], textures["grass1_r"],
		/*5 - 8*/textures["grass2_a"], textures["grass2_n"], textures["grass2_s"], textures["grass2_r"],

		/*9 -12*/textures["bumpRock_a"], textures["bumpRock_n"], textures["bumpRock_s"], textures["bumpRock_r"],
		/*13-16*/textures["dryRock_a"], textures["dryRock_n"], textures["dryRock_s"], textures["dryRock_r"],

		/*17-20*/textures["dunes_a"], textures["dunes_n"], textures["dunes_s"], textures["dunes_r"],
		/*21-24*/textures["sand_a"], textures["sand_n"], textures["sand_s"], textures["sand_r"],
		/*25-28*/textures["cobble_a"], textures["cobble_n"], textures["cobble_s"], textures["cobble_r"],

		/* 29 */ textures["sea_n"],
		/*30-33*/textures["snow_a"],textures["snow_n"], textures["snow_s"], textures["snow_r"],
		/*34-36*/textures["snow2_a"], textures["snow2_n"], textures["snow2_s"],
		/*37-40*/textures["tech_a"], textures["tech_n"], textures["tech_s"], textures["tech_r"]
	};


	// <<< You could build materials (make sets of textures) here
	// <<< Then, user could make sets of materials and send them to a modelObject
}

float getFloorHeight(const glm::vec3& pos)
{
	glm::vec3 espheroid = glm::normalize(pos - planetGrid.nucleus) * planetGrid.radius;
	return 1.70 + planetGrid.radius + noiser_2.GetNoise(espheroid.x, espheroid.y, espheroid.z);
}

void setPoints(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	Icosahedron icos(30.f);	// Just created for calling destructor, which applies a multiplier.
	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), Icosahedron::icos.size() / 6, Icosahedron::icos.data(), noIndices, false);

	assets["points"] = app.newModel(
		1, 1, primitiveTopology::point,
		vertexLoader,
		1, 3 * mat4size,	// M, V, P
		0,
		noTextures,
		shaders["v_point"], shaders["f_point"],
		false);

	memcpy(assets["points"]->vsDynUBO.getUBOptr(0), &modelMatrix(), mat4size);
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
		1, 3 * mat4size,	// M, V, P
		0,
		noTextures,
		shaders["v_line"], shaders["f_line"],
		false);

	memcpy(assets["axis"]->vsDynUBO.getUBOptr(0), &modelMatrix(), mat4size);
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
		1, 3 * mat4size,	// M, V, P
		0,
		noTextures,
		shaders["v_line"], shaders["f_line"],
		false);
}

void setSkybox(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<texIterator> usedTextures = { textures["skybox"] };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), 14, v_cube.data(), i_inCube, false);

	assets["skyBox"] = app.newModel(
		0, 1, primitiveTopology::triangle,
		vertexLoader,
		1, 3 * mat4size,	// M, V, P
		0,
		usedTextures,
		shaders["v_skybox"], shaders["f_skybox"],
		false);
}

void setCottage(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	// Add a model to render. An iterator is returned (modelIterator). Save it for updating model data later.
	std::vector<texIterator> usedTextures = { textures["cottage"] };

	VertexLoader* vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (vertexDir + "cottage_obj.obj").c_str());

	assets["cottage"] = app.newModel(			// TEST (before render loop): newModel
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		1, 3 * mat4size,	// M, V, P
		0,
		usedTextures,
		shaders["v_house"], shaders["f_house"],
		false);

	// Delete a model you passed previously.
	app.deleteModel(assets["cottage"]);			// TEST (before render loop): deleteModel

	vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (vertexDir + "cottage_obj.obj").c_str());

	assets["cottage"] = app.newModel(
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		1, 3 * mat4size,	// M, V, P
		0,
		usedTextures,
		shaders["v_house"], shaders["f_house"],
		false);
}

void setRoom(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<texIterator> usedTextures = { textures["room"] };

	VertexLoader* vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (vertexDir + "viking_room.obj").c_str());

	assets["room"] = app.newModel(
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		2, 3 * mat4size,	// M, V, P
		0,
		usedTextures,
		shaders["v_house"], shaders["f_house"],
		false);

	app.setRenders(assets["room"], 2);	// TEST (before render loop): setRenders
	app.setRenders(assets["room"], 3);	// TEST(in render loop) : setRenders(out of range)

	memcpy(assets["room"]->vsDynUBO.getUBOptr(0), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, -90.f), glm::vec3( 0.f, -50.f, 3.f)), mat4size);
	memcpy(assets["room"]->vsDynUBO.getUBOptr(1), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f,   0.f), glm::vec3( 0.f, -80.f, 3.f)), mat4size);
	memcpy(assets["room"]->vsDynUBO.getUBOptr(2), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f,  90.f), glm::vec3(30.f, -80.f, 3.f)), mat4size);
	//memcpy(assets["room"]->vsDynUBO.getUBOptr(3), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, 180.f), glm::vec3(30.f, -50.f, 3.f)), mat4size);
}

void setSea(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<texIterator> usedTextures = { textures["sea_n"] };

	float extent = 2000, height = 0;
	float v_sea[4 * 6] = {
		-extent, extent, height,  0, 0, 1,
		-extent,-extent, height,  0, 0, 1,
		 extent,-extent, height,  0, 0, 1,
		 extent, extent, height,  0, 0, 1 };
	std::vector<uint16_t> i_sea = { 0,1,3, 1,2,3 };

	//std::vector<VertexPT> v_sea;
	//std::vector<uint16_t> i_sea;
	//size_t numVertex = getPlane(v_sea, i_sea, 2000.f, 2000.f, 20.f);
	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 0, 1), 4, v_sea, i_sea, true);

	assets["sea"] = app.newModel(
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		1, 4 * mat4size + vec4size + 2 * sizeof(LightPosDir),	// M, V, P, MN, camPos, Light
		vec4size + 2 * sizeof(LightProps),						// Time, 2 * LightProps (6*vec4)
		usedTextures,
		shaders["v_sea"], shaders["f_sea"],
		true);
}

void setChunk(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	updateChunk = true;

	singleChunk.computeTerrain(true, 1.f);
	singleChunk.render(shaders["v_terrain"], shaders["f_terrain"], usedTextures, nullptr);
}

void setChunkGrid(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	updateChunkGrid = true;

	terrGrid.addTextures(usedTextures);
	terrGrid.addShaders(shaders["v_terrain"], shaders["f_terrain"]);
	//terrChunks.updateTree(glm::vec3(0,0,0));
}

void setSun(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<VertexPT> v_sun;
	std::vector<uint16_t> i_sun;
	size_t numVertex = getPlane(v_sun, i_sun, 1.f, 1.f, 0.f);		// LOOK dynamic adjustment of reticule size when window is resized

	std::vector<texIterator> usedTextures = { textures["sun"] };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), numVertex, v_sun.data(), i_sun, true);

	assets["sun"] = app.newModel(
		0, 1, primitiveTopology::triangle,
		vertexLoader,
		1, 3 * mat4size,	// M, V, P
		0,
		usedTextures,
		shaders["v_sun"], shaders["f_sun"],
		true);
}

void setReticule(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	//vertexDestination = std::vector<VertexPT>{
	//VertexPT(glm::vec3(-horSize / 2, -vertSize / 2, 0.f), glm::vec2(0, 0)),
	//VertexPT(glm::vec3(-horSize / 2,  vertSize / 2, 0.f), glm::vec2(0, 1)),
	//VertexPT(glm::vec3(horSize / 2,  vertSize / 2, 0.f), glm::vec2(1, 1)),
	//VertexPT(glm::vec3(horSize / 2, -vertSize / 2, 0.f), glm::vec2(1, 0)) };
	//
	//indicesDestination = std::vector<uint16_t>{ 0, 1, 3,  1, 2, 3 };

	float siz = 0.1;
	float v_ret[4 * 5] =
	{
		-siz,-siz, 0,  0, 0,
		-siz, siz, 0,  0, 1,
		 siz, siz, 0,  1, 1,
		 siz,-siz, 0,  1, 0
	};
	std::vector<uint16_t> i_ret = { 0,1,3, 1,2,3 };

	std::vector<texIterator> usedTextures = { textures["reticule"] };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), 4, v_ret, i_ret, true);

	assets["reticule"] = app.newModel(
		2, 1, primitiveTopology::triangle,
		vertexLoader,
		1, vec4size,				// aspect ratio (float)
		0,
		usedTextures,
		shaders["v_hud"], shaders["f_hud"],
		true);
}
