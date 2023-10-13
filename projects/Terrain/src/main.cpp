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
void loadResourcesInfo();
//void setLights();
//float getFloorHeight(const glm::vec3& pos);
//float getSeaHeight(const glm::vec3& pos);
//void tests();

// Models, textures, & shaders
//std::map<std::string, modelIter> assets;	// Model iterators

EntityManager em;	// world
std::map<std::string, ShaderLoader> shaderLoaders;
std::map<std::string, TextureLoader> texInfos;
std::vector<TextureLoader> planetTexInfos;	// Package of textures

// main ---------------------------------------------------------------------

int main(int argc, char* argv[])
{
	#ifdef DEBUG_MAIN
		//std::cout << std::setprecision(7);
		std::cout << "Current path: " << std::filesystem::current_path() << std::endl;
		TimerSet time;
		std::cout << "--------------------" << std::endl << time.getDate() << std::endl;
	#endif

	try   // https://www.tutorialspoint.com/cplusplus/cpp_exceptions_handling.htm
	{
		IOmanager io(1920/2, 1080/2);
		Renderer app(update, io, 2);		// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
		EntityFactory eFact(app);
		bool withPP = false;				// Add Post-Processing effects (atmosphere...) or not

		loadResourcesInfo();
		
		// ENTITIES + COMPONENTS:
		
		em.addEntity(std::vector<Component*>{	// Singleton components.
			new c_Engine(app),
			new c_Input,
			new c_Camera(3),
			new c_Sky(0.0035, 0, 0.0035+0.00028, 0, 40),
			new c_Lights(3) });
		//em.addEntity(eFact.createPoints(shaderLoaders[0], shaderLoaders[1], { }));	// <<<
		em.addEntity(eFact.createAxes(shaderLoaders["v_lines"], shaderLoaders["f_lines"], { }));
		//em.addEntity(eFact.createGrid(shaderLoaders["v_lines"], shaderLoaders["f_lines"], { }));
		em.addEntity(eFact.createSkyBox(shaderLoaders["v_skybox"], shaderLoaders["f_skybox"], {texInfos["space_1"]}));
		em.addEntity(eFact.createSun(shaderLoaders["v_sun"], shaderLoaders["f_sun"], { texInfos["sun"] }));
		//em.addEntity(eFact.createSphere(shaderLoaders["v_seaPlanet"], shaderLoaders["f_seaPlanet"], planetTexInfos));
		em.addEntity(eFact.createPlanet(shaderLoaders["v_planetChunk"], shaderLoaders["f_planetChunk"], planetTexInfos));
		em.addEntity(eFact.createGrass(shaderLoaders["v_grass"], shaderLoaders["f_grass"], { texInfos["grass0"], texInfos["grass1"], texInfos["grass2"], texInfos["whiteNoise"] }, (c_Lights*)em.getSComponent(CT::lights)));
		if(withPP) em.addEntity(eFact.createAtmosphere(shaderLoaders["v_atmosphere"], shaderLoaders["f_atmosphere"]));
		else em.addEntity(eFact.createNoPP(shaderLoaders["v_noPP"], shaderLoaders["f_noPP"], { texInfos["sun"], texInfos["hud"] }));
		
		// SYSTEMS:

		em.addSystem(new s_Engine);
		em.addSystem(new s_Input);
		em.addSystem(new s_PlaneCam);	// s_SphereCam, s_PolarCam, s_PlaneCam, s_FPCam
		em.addSystem(new s_Sky_XY);		// s_Sky_XY, s_Sky_XZ
		em.addSystem(new s_Lights);
		em.addSystem(new s_Move);
		em.addSystem(new s_ModelMatrix);
		em.addSystem(new s_Model);		// update UBOs
		
		#ifdef DEBUG_MAIN
			world.printInfo();
			std::cout << "--------------------" << std::endl;
		#endif
		
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

void update(Renderer& rend, glm::mat4 view, glm::mat4 proj)
{
	//d.frameTime = (float)(rend.getTimer().getTime());
	//d.fps = rend.getTimer().getFPS();
	//d.maxfps = rend.getTimer().getMaxPossibleFPS();
	//d.groundHeight = planetGrid.getGroundHeight(d.camPos);
	std::cout << rend.getMaxMemoryAllocationCount() << " / " << rend.getMemAllocObjects() << std::endl;

	em.update(rend.getTimer().getDeltaTime());
}

void loadResourcesInfo()
{
	// SHADERS

	//shaderLoaders["v_points"] = ShaderLoader(shadersDir + "v_points.vert");
	shaderLoaders.insert(std::pair("v_points",      ShaderLoader(shadersDir + "v_points.vert")));
	shaderLoaders.insert(std::pair("f_points",      ShaderLoader(shadersDir + "f_points.frag")));

	shaderLoaders.insert(std::pair("v_lines",       ShaderLoader(shadersDir + "v_lines.vert")));
	shaderLoaders.insert(std::pair("f_lines",       ShaderLoader(shadersDir + "f_lines.frag")));
	
	shaderLoaders.insert(std::pair("v_skybox",      ShaderLoader(shadersDir + "v_skybox.vert")));
	shaderLoaders.insert(std::pair("f_skybox",      ShaderLoader(shadersDir + "f_skybox.frag")));

	shaderLoaders.insert(std::pair("v_basicModel",  ShaderLoader(shadersDir + "v_basicModel.vert")));
	shaderLoaders.insert(std::pair("f_basicModel",  ShaderLoader(shadersDir + "f_basicModel.frag")));

	shaderLoaders.insert(std::pair("v_grass",       ShaderLoader(shadersDir + "v_grass.vert")));
	shaderLoaders.insert(std::pair("f_grass",       ShaderLoader(shadersDir + "f_grass.frag")));

	shaderLoaders.insert(std::pair("v_seaPlanet",   ShaderLoader(shadersDir + "v_seaPlanet.vert")));
	shaderLoaders.insert(std::pair("f_seaPlanet",   ShaderLoader(shadersDir + "f_seaPlanet.frag")));

	shaderLoaders.insert(std::pair("v_plainChunk",  ShaderLoader(shadersDir + "v_plainChunk.vert")));
	shaderLoaders.insert(std::pair("f_plainChunk",  ShaderLoader(shadersDir + "f_plainChunk.frag")));

	shaderLoaders.insert(std::pair("v_planetChunk", ShaderLoader(shadersDir + "v_planetChunk.vert")));
	shaderLoaders.insert(std::pair("f_planetChunk", ShaderLoader(shadersDir + "f_planetChunk.frag")));

	shaderLoaders.insert(std::pair("v_sun",         ShaderLoader(shadersDir + "v_sun.vert")));
	shaderLoaders.insert(std::pair("f_sun",         ShaderLoader(shadersDir + "f_sun.frag")));

	shaderLoaders.insert(std::pair("v_hud",         ShaderLoader(shadersDir + "v_hud.vert")));
	shaderLoaders.insert(std::pair("f_hud",         ShaderLoader(shadersDir + "f_hud.frag")));

	shaderLoaders.insert(std::pair("v_atmosphere",  ShaderLoader(shadersDir + "v_atmosphere.vert")));
	shaderLoaders.insert(std::pair("f_atmosphere",  ShaderLoader(shadersDir + "f_atmosphere.frag")));

	shaderLoaders.insert(std::pair("v_noPP",        ShaderLoader(shadersDir + "v_noPP.vert")));
	shaderLoaders.insert(std::pair("f_noPP",        ShaderLoader(shadersDir + "f_noPP.frag")));

	// TEXTURES

	// Special
	texInfos.insert(std::pair("space_1",   TextureLoader(texDir + "sky_box/space1.jpg")));
	texInfos.insert(std::pair("cottage_d", TextureLoader(texDir + "models/cottage/cottage_diffuse.png")));
	texInfos.insert(std::pair("room",      TextureLoader(texDir + "models/viking_room.png")));
	texInfos.insert(std::pair("squares",   TextureLoader(texDir + "squares.png")));
	texInfos.insert(std::pair("sun",       TextureLoader(texDir + "Sun/sun2_1.png")));
	texInfos.insert(std::pair("hud",       TextureLoader(texDir + "HUD/reticule_1.png")));

	// Plants
	texInfos.insert(std::pair("grassDry_a", TextureLoader(texDir + "grass/grassDry_a.png")));
	texInfos.insert(std::pair("grassDry_n", TextureLoader(texDir + "grass/grassDry_n.png")));
	texInfos.insert(std::pair("grassDry_s", TextureLoader(texDir + "grass/grassDry_s.png")));
	texInfos.insert(std::pair("grassDry_r", TextureLoader(texDir + "grass/grassDry_r.png")));
	texInfos.insert(std::pair("grassDry_h", TextureLoader(texDir + "grass/grassDry_h.png")));

	// Rocks
	texInfos.insert(std::pair("bumpRock_a", TextureLoader(texDir + "rock/bumpRock_a.png")));
	texInfos.insert(std::pair("bumpRock_n", TextureLoader(texDir + "rock/bumpRock_n.png")));
	texInfos.insert(std::pair("bumpRock_s", TextureLoader(texDir + "rock/bumpRock_s.png")));
	texInfos.insert(std::pair("bumpRock_r", TextureLoader(texDir + "rock/bumpRock_r.png")));
	texInfos.insert(std::pair("bumpRock_h", TextureLoader(texDir + "rock/bumpRock_h.png")));

	// Soils
	texInfos.insert(std::pair("sandDunes_a", TextureLoader(texDir + "sand/sandDunes_a.png")));
	texInfos.insert(std::pair("sandDunes_n", TextureLoader(texDir + "sand/sandDunes_n.png")));
	texInfos.insert(std::pair("sandDunes_s", TextureLoader(texDir + "sand/sandDunes_s.png")));
	texInfos.insert(std::pair("sandDunes_r", TextureLoader(texDir + "sand/sandDunes_r.png")));
	texInfos.insert(std::pair("sandDunes_h", TextureLoader(texDir + "sand/sandDunes_h.png")));

	texInfos.insert(std::pair("sandWavy_a", TextureLoader(texDir + "sand/sandWavy_a.png")));
	texInfos.insert(std::pair("sandWavy_n", TextureLoader(texDir + "sand/sandWavy_n.png")));
	texInfos.insert(std::pair("sandWavy_s", TextureLoader(texDir + "sand/sandWavy_s.png")));
	texInfos.insert(std::pair("sandWavy_r", TextureLoader(texDir + "sand/sandWavy_r.png")));
	texInfos.insert(std::pair("sandWavy_h", TextureLoader(texDir + "sand/sandWavy_h.png")));

	// Water
	texInfos.insert(std::pair("sea_n",      TextureLoader(texDir + "water/sea_n.png")));
	texInfos.insert(std::pair("sea_h",      TextureLoader(texDir + "water/sea_h.png")));
	texInfos.insert(std::pair("sea_foam_a", TextureLoader(texDir + "water/sea_foam_a.png")));

	// Snow
	texInfos.insert(std::pair("snow_a", TextureLoader(texDir + "snow/snow_a.png")));
	texInfos.insert(std::pair("snow_n", TextureLoader(texDir + "snow/snow_n.png")));
	texInfos.insert(std::pair("snow_s", TextureLoader(texDir + "snow/snow_s.png")));
	texInfos.insert(std::pair("snow_r", TextureLoader(texDir + "snow/snow_r.png")));
	texInfos.insert(std::pair("snow_h", TextureLoader(texDir + "snow/snow_h.png")));

	texInfos.insert(std::pair("snow2_a", TextureLoader(texDir + "snow/snow2_a.png")));
	texInfos.insert(std::pair("snow2_n", TextureLoader(texDir + "snow/snow2_n.png")));
	texInfos.insert(std::pair("snow2_s", TextureLoader(texDir + "snow/snow2_s.png")));

	// Others
	texInfos.insert(std::pair("grass0",     TextureLoader(texDir + "grass/grass0.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
	texInfos.insert(std::pair("grass1",     TextureLoader(texDir + "grass/grass1.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
	texInfos.insert(std::pair("grass2",     TextureLoader(texDir + "grass/grass2.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
	texInfos.insert(std::pair("whiteNoise", TextureLoader(texDir + "grass/whiteNoise.png")));

	// TEXTURE PACKS

	// Plants
	planetTexInfos.push_back(texInfos["grassDry_a"]);
	planetTexInfos.push_back(texInfos["grassDry_n"]);
	planetTexInfos.push_back(texInfos["grassDry_s"]);
	planetTexInfos.push_back(texInfos["grassDry_r"]);
	planetTexInfos.push_back(texInfos["grassDry_h"]);
	// Rocks
	planetTexInfos.push_back(texInfos["bumpRock_a"]);
	planetTexInfos.push_back(texInfos["bumpRock_n"]);
	planetTexInfos.push_back(texInfos["bumpRock_s"]);
	planetTexInfos.push_back(texInfos["bumpRock_r"]);
	planetTexInfos.push_back(texInfos["bumpRock_h"]);
	// Snow
	planetTexInfos.push_back(texInfos["snow_a"]);
	planetTexInfos.push_back(texInfos["snow_n"]);
	planetTexInfos.push_back(texInfos["snow_s"]);
	planetTexInfos.push_back(texInfos["snow_r"]);
	planetTexInfos.push_back(texInfos["snow_h"]);
	planetTexInfos.push_back(texInfos["snow2_a"]);
	planetTexInfos.push_back(texInfos["snow2_n"]);
	planetTexInfos.push_back(texInfos["snow2_s"]);
	planetTexInfos.push_back(texInfos["snow_r"]);	// repeated
	planetTexInfos.push_back(texInfos["snow_h"]);	// repeated
	// Soils
	planetTexInfos.push_back(texInfos["sandDunes_a"]);
	planetTexInfos.push_back(texInfos["sandDunes_n"]);
	planetTexInfos.push_back(texInfos["sandDunes_s"]);
	planetTexInfos.push_back(texInfos["sandDunes_r"]);
	planetTexInfos.push_back(texInfos["sandDunes_h"]);
	planetTexInfos.push_back(texInfos["sandWavy_a"]);
	planetTexInfos.push_back(texInfos["sandWavy_n"]);
	planetTexInfos.push_back(texInfos["sandWavy_s"]);
	planetTexInfos.push_back(texInfos["sandWavy_r"]);
	planetTexInfos.push_back(texInfos["sandWavy_h"]);
	planetTexInfos.push_back(texInfos["squares"]);
	// Water
	planetTexInfos.push_back(texInfos["sea_n"]);
	planetTexInfos.push_back(texInfos["sea_h"]);
	planetTexInfos.push_back(texInfos["sea_foam_a"]);
}