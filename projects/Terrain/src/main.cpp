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
void setSea(Renderer& app);
void setSkybox(Renderer& app);
void setCottage(Renderer& app);
void setRoom(Renderer& app);
void setChunk(Renderer& app);
void setChunkGrid(Renderer& app);
void setSphereChunks(Renderer& app);
void setSphereGrid(Renderer& app);
void setSun(Renderer& app);
void setReticule(Renderer& app);

// Models & textures
Renderer app(update, &camera_1, 3);				// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
std::map<std::string, modelIterator> assets;	// Model iterators
std::map<std::string, texIterator> textures;	// Texture iterators

// Others
int gridStep = 50;
ifOnce check;			// LOOK implement as functor (function with state)
std::vector<Light*> lights = { &sunLight };

// Terrain
Noiser noiser_1(	// Desert
	FastNoiseLite::NoiseType_Cellular,	// Noise type
	4, 1.5, 0.28f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
	1, 70,								// Scale, Multiplier
	0,									// Curve degree
	500, 500, 0,						// XYZ offsets
	4952);								// Seed

Noiser noiser_2(	// Hills
	FastNoiseLite::NoiseType_Perlin,	// Noise type
	8, 10., 0.2f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
	0.8, 50,								// Scale, Multiplier
	0,									// Curve degree
	500, 500, 0,						// XYZ offsets
	4952);								// Seed

PlainChunk singleChunk(app, noiser_1, lights, glm::vec3(100, 25, 0), 5, 41, 11);

TerrainGrid terrGrid(app, noiser_1, lights, 6400, 21, 8, 2, 1.2);

SphericalChunk sphereChunk_pX(app, noiser_1, glm::vec3( 50,  0,  0), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 1, 0, 0));
SphericalChunk sphereChunk_nX(app, noiser_1, glm::vec3(-50,  0,  0), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3(-1, 0, 0));
SphericalChunk sphereChunk_pY(app, noiser_1, glm::vec3(  0, 50,  0), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 1, 0));
SphericalChunk sphereChunk_nY(app, noiser_1, glm::vec3(  0,-50,  0), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 0,-1, 0));
SphericalChunk sphereChunk_pZ(app, noiser_1, glm::vec3(  0,  0, 50), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 0, 1));
SphericalChunk sphereChunk_nZ(app, noiser_1, glm::vec3(  0,  0,-50), 1, 101, 101, lights, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 0,-1));

PlanetGrid planetGrid_pZ(app, noiser_1, lights, 100, 21, 8, 2, 1.2, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 0, 1), glm::vec3(  0,  0, 50));
PlanetGrid planetGrid_nZ(app, noiser_1, lights, 100, 21, 8, 2, 1.2, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 0,-1), glm::vec3(  0,  0,-50));
PlanetGrid planetGrid_pY(app, noiser_1, lights, 100, 21, 8, 2, 1.2, 1000, glm::vec3(0, 0, 0), glm::vec3( 0, 1, 0), glm::vec3(  0, 50,  0));
PlanetGrid planetGrid_nY(app, noiser_1, lights, 100, 21, 8, 2, 1.2, 1000, glm::vec3(0, 0, 0), glm::vec3( 0,-1, 0), glm::vec3(  0,-50,  0));
PlanetGrid planetGrid_pX(app, noiser_1, lights, 100, 21, 8, 2, 1.2, 1000, glm::vec3(0, 0, 0), glm::vec3( 1, 0, 0), glm::vec3( 50,  0,  0));
PlanetGrid planetGrid_nX(app, noiser_1, lights, 100, 21, 8, 2, 1.2, 1000, glm::vec3(0, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(-50,  0,  0));

bool updateChunk = false, updateChunkSet = false, updatePlanet = false, updatePlanetSet = false;

// Data to update
long double frameTime;
size_t fps;
size_t maxfps;
glm::vec3 camPos;


int main(int argc, char* argv[])
{
	TimerSet time;
	std::cout << "------------------------------" << std::endl << time.getDate() << std::endl;

	setLights();
	loadTextures(app);

	setPoints(app);
	setAxis(app);
	//setGrid(app);
	setSea(app);
	setSkybox(app);
	//setCottage(app);
	//setRoom(app);
	//setChunk(app);
	setChunkGrid(app);
	//setSphereChunks(app);
	//setSphereGrid(app);
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
	camPos		= rend.getCamera().camPos;
	size_t i;
	
	std::cout << rend.getFrameCount() << ") \n";
	//std::cout << ") \n  Commands: " << rend.getCommandsCount() / 3 << std::endl;

	dayTime = 5.00 + frameTime * 0.5;
	sunLight.setDirectional(Sun::lightDirection(dayTime), glm::vec3(0.03, 0.03, 0.03), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));

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
	if (updateChunk) singleChunk.updateUBOs(camPos, view, proj);

	if(updateChunkSet)
	{
		std::cout << "  Nodes: " << terrGrid.getRenderedChunks() << '/' << terrGrid.getloadedChunks() << '/' << terrGrid.getTotalNodes() << std::endl;
		terrGrid.updateTree(camPos);
		terrGrid.updateUBOs(camPos, view, proj);
	}

	if (updatePlanet)
	{
		sphereChunk_pX.updateUBOs(camPos, view, proj);
		sphereChunk_nX.updateUBOs(camPos, view, proj);
		sphereChunk_pY.updateUBOs(camPos, view, proj);
		sphereChunk_nY.updateUBOs(camPos, view, proj);
		sphereChunk_pZ.updateUBOs(camPos, view, proj);
		sphereChunk_nZ.updateUBOs(camPos, view, proj);
	}

	if (updatePlanetSet)
	{
		//std::cout << "  Nodes: " << planetGrid_pZ.getloadedChunks() << '/' << planetGrid_pZ.getRenderedChunks() << '/' << planetGrid_pZ.getTotalNodes() << std::endl;
		planetGrid_pZ.updateTree(camPos);
		planetGrid_pZ.updateUBOs(camPos, view, proj);
		planetGrid_nZ.updateTree(camPos);
		planetGrid_nZ.updateUBOs(camPos, view, proj);
		planetGrid_pY.updateTree(camPos);
		planetGrid_pY.updateUBOs(camPos, view, proj);
		planetGrid_nY.updateTree(camPos);
		planetGrid_nY.updateUBOs(camPos, view, proj);
		planetGrid_pX.updateTree(camPos);
		planetGrid_pX.updateUBOs(camPos, view, proj);
		planetGrid_nX.updateTree(camPos);
		planetGrid_nX.updateUBOs(camPos, view, proj);
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
		memcpy(assets["points"]->vsDynUBO.getUBOptr(0), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, -90.f), glm::vec3( 0.f, -50.f, 3.f)), mat4size);
		memcpy(assets["points"]->vsDynUBO.getUBOptr(1), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f,   0.f), glm::vec3( 0.f, -80.f, 3.f)), mat4size);
		memcpy(assets["points"]->vsDynUBO.getUBOptr(2), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f,  90.f), glm::vec3(30.f, -80.f, 3.f)), mat4size);
		memcpy(assets["points"]->vsDynUBO.getUBOptr(3), &modelMatrix(glm::vec3(20.f, 20.f, 20.f), glm::vec3(0.f, 0.f, 180.f), glm::vec3(30.f, -50.f, 3.f)), mat4size);
	}
*/
	// Update UBOs
	uint8_t* dest;

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
		for (i = 0; i < assets["sun"]->vsDynUBO.numDynUBOs; i++) 
		{
			dest = assets["sea"]->vsDynUBO.getUBOptr(0);
			memcpy(dest + 0 * mat4size, &modelMatrix(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(camPos.x, camPos.y, 0)), mat4size);
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
			memcpy(dest + 3 * mat4size, &modelMatrixForNormals(modelMatrix()), mat4size);
			memcpy(dest + 4 * mat4size, &camPos, vec3size);
			memcpy(dest + 4 * mat4size + vec4size, &sunLight, sizeof(Light));
			float fTime = (float)frameTime;
			memcpy(assets["sea"]->fsUBO.getUBOptr(0), &fTime, sizeof(fTime));
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
	std::cout << "> " << __func__ << "()" << std::endl;

	//sunLight.turnOff();
	sunLight.setDirectional(Sun::lightDirection(dayTime), glm::vec3(0.03, 0.03, 0.03), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));
	//sunLight.setPoint(glm::vec3(0, 0, 50), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 0.1, 0.01);
	//sunLight.setSpot(glm::vec3(0, 0, 150), glm::vec3(0, 0, 1), glm::vec3(0.1, 0.1, 0.1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), 1, 0, 0., 0.9, 0.8);
}

void loadTextures(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	// Special
	textures["skybox"]	 = app.newTexture((TEXTURES_DIR + "sky_box/space1.jpg").c_str());
	textures["cottage"]  = app.newTexture((TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str());
	textures["room"]	 = app.newTexture((TEXTURES_DIR + "viking_room.png").c_str());
	textures["squares"]  = app.newTexture((TEXTURES_DIR + "squares.png").c_str());
	textures["sun"]		 = app.newTexture((TEXTURES_DIR + "Sun/sun2_1.png").c_str());
	textures["reticule"] = app.newTexture((TEXTURES_DIR + "HUD/reticule_1.png").c_str());
	app.deleteTexture(textures["skybox"]);													// TEST (before render loop): deleteTexture
	textures["skybox"]	 = app.newTexture((TEXTURES_DIR + "sky_box/space1.jpg").c_str());	// TEST (before render loop): newTexture

	// Plants
	textures["green_a"] = app.newTexture((TEXTURES_DIR + "green_a.png").c_str());
	textures["green_n"] = app.newTexture((TEXTURES_DIR + "green_n.png").c_str());
	textures["green_s"] = app.newTexture((TEXTURES_DIR + "green_s.png").c_str());
	textures["green_r"] = app.newTexture((TEXTURES_DIR + "green_r.png").c_str());

	textures["grass_a"] = app.newTexture((TEXTURES_DIR + "grass_a.png").c_str());
	textures["grass_n"] = app.newTexture((TEXTURES_DIR + "grass_n.png").c_str());
	textures["grass_s"] = app.newTexture((TEXTURES_DIR + "grass_s.png").c_str());
	textures["grass_r"] = app.newTexture((TEXTURES_DIR + "grass_r.png").c_str());

	// Rocks
	textures["bumpRock_a"] = app.newTexture((TEXTURES_DIR + "bumpRock_a.png").c_str());
	textures["bumpRock_n"] = app.newTexture((TEXTURES_DIR + "bumpRock_n.png").c_str());
	textures["bumpRock_s"] = app.newTexture((TEXTURES_DIR + "bumpRock_s.png").c_str());
	textures["bumpRock_r"] = app.newTexture((TEXTURES_DIR + "bumpRock_r.png").c_str());

	textures["dryRock_a"]  = app.newTexture((TEXTURES_DIR + "dryRock_a.png").c_str());
	textures["dryRock_n"]  = app.newTexture((TEXTURES_DIR + "dryRock_n.png").c_str());
	textures["dryRock_s"]  = app.newTexture((TEXTURES_DIR + "dryRock_s.png").c_str());
	textures["dryRock_r"]  = app.newTexture((TEXTURES_DIR + "dryRock_r.png").c_str());

	// Soils
	textures["dunes_a"]  = app.newTexture((TEXTURES_DIR + "dunes_a.png").c_str());
	textures["dunes_n"]  = app.newTexture((TEXTURES_DIR + "dunes_n.png").c_str());
	textures["dunes_s"]  = app.newTexture((TEXTURES_DIR + "dunes_s.png").c_str());
	textures["dunes_r"]  = app.newTexture((TEXTURES_DIR + "dunes_r.png").c_str());

	textures["sand_a"]   = app.newTexture((TEXTURES_DIR + "sand_a.png").c_str());
	textures["sand_n"]   = app.newTexture((TEXTURES_DIR + "sand_n.png").c_str());
	textures["sand_s"]   = app.newTexture((TEXTURES_DIR + "sand_s.png").c_str());
	textures["sand_r"]   = app.newTexture((TEXTURES_DIR + "sand_r.png").c_str());

	textures["cobble_a"] = app.newTexture((TEXTURES_DIR + "cobble_a.png").c_str());
	textures["cobble_n"] = app.newTexture((TEXTURES_DIR + "cobble_n.png").c_str());
	textures["cobble_s"] = app.newTexture((TEXTURES_DIR + "cobble_s.png").c_str());
	textures["cobble_r"] = app.newTexture((TEXTURES_DIR + "cobble_r.png").c_str());

	// Water
	textures["sea_n"]   = app.newTexture((TEXTURES_DIR + "sea_n.png").c_str());
					    
	textures["snow_a"]  = app.newTexture((TEXTURES_DIR + "snow_a.png").c_str());
	textures["snow_n"]  = app.newTexture((TEXTURES_DIR + "snow_n.png").c_str());
	textures["snow_s"]  = app.newTexture((TEXTURES_DIR + "snow_s.png").c_str());
	textures["snow_r"]  = app.newTexture((TEXTURES_DIR + "snow_r.png").c_str());

	textures["snow2_a"] = app.newTexture((TEXTURES_DIR + "snow2_a.png").c_str());
	textures["snow2_n"] = app.newTexture((TEXTURES_DIR + "snow2_n.png").c_str());
	textures["snow2_s"] = app.newTexture((TEXTURES_DIR + "snow2_s.png").c_str());

	// Others

	textures["tech_a"] = app.newTexture((TEXTURES_DIR + "tech_a.png").c_str());
	textures["tech_n"] = app.newTexture((TEXTURES_DIR + "tech_n.png").c_str());
	textures["tech_s"] = app.newTexture((TEXTURES_DIR + "tech_s.png").c_str());
	textures["tech_r"] = app.newTexture((TEXTURES_DIR + "tech_r.png").c_str());

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
		1, 3 * mat4size,	// M, V, P
		0,
		noTextures,
		(SHADERS_DIR + "v_pointPC.spv").c_str(),
		(SHADERS_DIR + "f_pointPC.spv").c_str(),
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
		(SHADERS_DIR + "v_linePC.spv").c_str(),
		(SHADERS_DIR + "f_linePC.spv").c_str(),
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
		(SHADERS_DIR + "v_linePC.spv").c_str(),
		(SHADERS_DIR + "f_linePC.spv").c_str(),
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
		(SHADERS_DIR + "v_trianglePT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePT.spv").c_str(),
		false);
}

void setCottage(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	// Add a model to render. An iterator is returned (modelIterator). Save it for updating model data later.
	std::vector<texIterator> usedTextures = { textures["cottage"] };

	VertexLoader* vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (MODELS_DIR + "cottage_obj.obj").c_str());

	assets["cottage"] = app.newModel(			// TEST (before render loop): newModel
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		1, 3 * mat4size,	// M, V, P
		0,
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
		1, 3 * mat4size,	// M, V, P
		0,
		usedTextures,
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
		false);
}

void setRoom(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<texIterator> usedTextures = { textures["room"] };

	VertexLoader* vertexLoader = new VertexFromFile(VertexType(1, 1, 1, 0), (MODELS_DIR + "viking_room.obj").c_str());

	assets["room"] = app.newModel(
		1, 1, primitiveTopology::triangle,
		vertexLoader,
		2, 3 * mat4size,	// M, V, P
		0,
		usedTextures,
		(SHADERS_DIR + "v_trianglePCT.spv").c_str(),
		(SHADERS_DIR + "f_trianglePCT.spv").c_str(),
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
		1, 4 * mat4size + vec4size + sizeof(Light),	// M, V, P, MN, camPos, Light
		vec4size,									// time
		usedTextures,
		(SHADERS_DIR + "v_sea.spv").c_str(),
		(SHADERS_DIR + "f_sea.spv").c_str(),
		false);
}

void setChunk(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	updateChunk = true;

	std::vector<texIterator> usedTextures = { textures["squares"], textures["grass"], textures["grassSpec"], textures["rock"], textures["rockSpec"], textures["sand"], textures["sandSpec"], textures["plainSand"], textures["plainSandSpec"] };

	singleChunk.computeTerrain(true);
	singleChunk.render((SHADERS_DIR + "v_terrainPTN.spv").c_str(), (SHADERS_DIR + "f_terrainPTN.spv").c_str(), usedTextures, nullptr);
}

void setChunkGrid(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	updateChunkSet = true;

	std::vector<texIterator> usedTextures = 
	{
		textures["squares"],

		textures["green_a"], textures["green_n"], textures["green_s"], textures["green_r"],
		textures["grass_a"], textures["grass_n"], textures["grass_s"], textures["grass_r"],

		textures["bumpRock_a"], textures["bumpRock_n"], textures["bumpRock_s"], textures["bumpRock_r"],
		textures["dryRock_a"], textures["dryRock_n"], textures["dryRock_s"], textures["dryRock_r"],

		textures["dunes_a"], textures["dunes_n"], textures["dunes_s"], textures["dunes_r"],
		textures["sand_a"], textures["sand_n"], textures["sand_s"], textures["sand_r"],
		textures["cobble_a"], textures["cobble_n"], textures["cobble_s"], textures["cobble_r"],

		textures["sea_n"],
		textures["snow_a"],textures["snow_n"], textures["snow_s"], textures["snow_r"],
		textures["snow2_a"], textures["snow2_n"], textures["snow2_s"],
		textures["tech_a"], textures["tech_n"], textures["tech_s"], textures["tech_r"]
	};

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

void setSphereGrid(Renderer& app)
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
	size_t numVertex = getPlane(v_sun, i_sun, 1.f, 1.f, 0.f);		// LOOK dynamic adjustment of reticule size when window is resized

	std::vector<texIterator> usedTextures = { textures["sun"] };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), numVertex, v_sun.data(), i_sun, true);

	assets["sun"] = app.newModel(
		0, 1, primitiveTopology::triangle,
		vertexLoader,
		1, 3 * mat4size,	// M, V, P
		0,
		usedTextures,
		(SHADERS_DIR + "v_sunPT.spv").c_str(),
		(SHADERS_DIR + "f_sunPT.spv").c_str(),
		true);
}

void setReticule(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	std::vector<VertexPT> v_ret;
	std::vector<uint16_t> i_ret;
	size_t numVertex = getPlaneNDC(v_ret, i_ret, 0.2f, 0.2f);		// LOOK dynamic adjustment of reticule size when window is resized

	std::vector<texIterator> usedTextures = { textures["reticule"] };

	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 0, 1, 0), numVertex, v_ret.data(), i_ret, true);

	assets["reticule"] = app.newModel(
		2, 1, primitiveTopology::triangle,
		vertexLoader,
		1, 0,
		0,
		usedTextures,
		(SHADERS_DIR + "v_hudPT.spv").c_str(),
		(SHADERS_DIR + "f_hudPT.spv").c_str(),
		true);
}
