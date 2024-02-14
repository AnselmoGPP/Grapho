#include <filesystem>
#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "toolkit.hpp"
#include "ECSarch.hpp"

#include "terrain.hpp"
#include "entities.hpp"
#include "components.hpp"
#include "systems.hpp"
#include "common.hpp"

// MVS executable's path        == Grapho\_BUILD\projects\Terrain (Terrain.sln)
// Standalone executable's path == Grapho\_BUILD\projects\Terrain\Release (Terrain.exe)
#define STANDALONE_EXECUTABLE false

#define NUM_LIGHTS 2
// Prototypes
void update(Renderer& rend);
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
UBOinfo globalUBOs[2];
std::map<std::string, VerticesLoader> verticesLoaders;

std::vector<TextureLoader> soilTexInfos;	// Package of textures
std::vector<TextureLoader> seaTexInfos;		// Package of textures
std::vector<TextureLoader> skyboxTexInfos;	// Package of textures

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
		loadResourcesInfo();						// Load shaders, textures, UBOs, meshes from files
		
		Renderer r(update, io, globalUBOs[0], globalUBOs[1]);		// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
		EntityFactory eFact(r);
		
		// ENTITIES + COMPONENTS:
		{
			em.addEntity("singletons", std::vector<Component*>{	// Singleton components.
				new c_Engine(r),
				new c_Input,
				new c_Cam_Plane_polar_sphere,	// Sphere, Plane_free, Plane_polar_sphere
				new c_Sky(0.0035, 0, 0.0035 + 0.00028, 0, 40),
				new c_Lights(NUM_LIGHTS) });

			// Geometry pass (deferred rendering)
			em.addEntity("planet", eFact.createPlanet(shaderLoaders, texInfos));
			em.addEntity("sea", eFact.createSphere(shaderLoaders, texInfos));
			em.addEntity("grass", eFact.createGrass(shaderLoaders, texInfos, verticesLoaders, (c_Lights*)em.getSComponent(CT::lights)));
			em.addEntity("plant", eFact.createPlant(shaderLoaders, texInfos, verticesLoaders, (c_Lights*)em.getSComponent(CT::lights)));
			em.addEntity("stone", eFact.createRock(shaderLoaders, texInfos, verticesLoaders, (c_Lights*)em.getSComponent(CT::lights)));
			em.addEntities(std::vector<std::string>{"trunk", "branch"}, eFact.createTree(shaderLoaders, texInfos, verticesLoaders, (c_Lights*)em.getSComponent(CT::lights)));
			em.addEntity("treeBB", eFact.createTreeBillboard(shaderLoaders, texInfos, verticesLoaders, (c_Lights*)em.getSComponent(CT::lights)));

			// Lighting pass (deferred rendering)
			em.addEntity("lightingPass", eFact.createLightingPass(shaderLoaders, texInfos, (c_Lights*)em.getSComponent(CT::lights)));
		
			// Forward pass (forward rendering)
			//em.addEntity(eFact.createPoints(shaderLoaders[0], shaderLoaders[1], { }));	// <<<
			//em.addEntity("axes", eFact.createAxes(shaderLoaders["v_lines"], shaderLoaders["f_lines"], {}));
			//em.addEntity("grid", eFact.createGrid(shaderLoaders["v_lines"], shaderLoaders["f_lines"], { }));
			//em.addEntity("skybox", eFact.createSkyBox(shaderLoaders["v_skybox"], shaderLoaders["f_skybox"], skyboxTexInfos));
			//em.addEntity("sun", eFact.createSun(shaderLoaders["v_sun"], shaderLoaders["f_sun"], { texInfos["sun"] }));
			//if (withPP) em.addEntity("atmosphere", eFact.createAtmosphere(shaderLoaders["v_atmosphere"], shaderLoaders["f_atmosphere"]));
			//else em.addEntity("noPP", eFact.createNoPP(shaderLoaders["v_noPP"], shaderLoaders["f_noPP"], { texInfos["sun"], texInfos["hud"] }));
		}
		// SYSTEMS:
		{
			em.addSystem(new s_Engine);
			em.addSystem(new s_Input);
			em.addSystem(new s_Camera);		// s_SphereCam (1), s_PolarCam (2), s_PlaneCam (3), s_FPCam (4)
			em.addSystem(new s_Sky_XY);		// s_Sky_XY, s_Sky_XZ
			em.addSystem(new s_Lights);
			em.addSystem(new s_Move);		// update model params
			em.addSystem(new s_Distributor);// update model params
			em.addSystem(new s_Model);		// update UBOs
		}

		#ifdef DEBUG_MAIN
			world.printInfo();
			std::cout << "--------------------" << std::endl;
		#endif
		
		r.renderLoop();		// Start rendering

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

void update(Renderer& rend)
{
	//d.frameTime = (float)(rend.getTimer().getTime());
	//d.fps = rend.getTimer().getFPS();
	//d.maxfps = rend.getTimer().getMaxPossibleFPS();
	//d.groundHeight = planetGrid.getGroundHeight(d.camPos);
	
	//std::cout << "MemAllocObjects: " << rend.getMaxMemoryAllocationCount() << " / " << rend.getMemAllocObjects() << std::endl;
	std::cout << rend.getTimer().getFPS() << '\n';
	
	em.update(rend.getTimer().getDeltaTime());
}

void loadResourcesInfo()
{
	// FILES' PATHS
	#if STANDALONE_EXECUTABLE
		#if defined(__unix__)
			const std::string shadersDir("../../../../projects/Terrain/shaders/GLSL/");
			const std::string vertexDir("../../../../models/");
			const std::string texDir("../../../../textures/");
		#elif _WIN64 || _WIN32
			const std::string shadersDir("../../../../projects/Terrain/shaders/GLSL/");
			const std::string vertexDir("../../../../models/");
			const std::string texDir("../../../../textures/");
		#endif
	#else
		#if defined(__unix__)
			const std::string shadersDir("../../../projects/Terrain/shaders/GLSL/");
			const std::string vertexDir("../../../models/");
			const std::string texDir("../../../textures/");
		#elif _WIN64 || _WIN32
			const std::string shadersDir("../../../projects/Terrain/shaders/GLSL/");
			const std::string vertexDir("../../../cg_resources/vertex/");
			const std::string texDir("../../../cg_resources/textures/");
		#endif
	#endif
	
	// GLOBAL UBOS
	globalUBOs[0] = UBOinfo(1, 1, size.mat4 + size.mat4 + size.vec4);			// View, Proj, camPos_Time
	globalUBOs[1] = UBOinfo(1, 1, size.vec4 + NUM_LIGHTS * sizeof(Light));		// camPos_Time, Lights

	// SHADERS
	{
		//shaderLoaders.insert(std::pair("v_points", ShaderLoader(shadersDir + "v_points.vert")));
		//shaderLoaders.insert(std::pair("f_points", ShaderLoader(shadersDir + "f_points.frag")));
		//
		//shaderLoaders.insert(std::pair("v_lines", ShaderLoader(shadersDir + "v_lines.vert")));
		//shaderLoaders.insert(std::pair("f_lines", ShaderLoader(shadersDir + "f_lines.frag")));
		//
		//shaderLoaders.insert(std::pair("v_skybox", ShaderLoader(shadersDir + "v_skybox.vert")));
		//shaderLoaders.insert(std::pair("f_skybox", ShaderLoader(shadersDir + "f_skybox.frag")));
		
		shaderLoaders.insert(std::pair("v_seaPlanet", ShaderLoader(shadersDir + "seaPlanet_v.vert")));
		shaderLoaders.insert(std::pair("f_seaPlanet", ShaderLoader(shadersDir + "seaPlanet_f.frag")));
		
		shaderLoaders.insert(std::pair("v_planetChunk", ShaderLoader(shadersDir + "planetChunk_v.vert")));
		shaderLoaders.insert(std::pair("f_planetChunk", ShaderLoader(shadersDir + "planetChunk_f.frag")));
		
		//shaderLoaders.insert(std::pair("v_sun", ShaderLoader(shadersDir + "v_sun.vert")));
		//shaderLoaders.insert(std::pair("f_sun", ShaderLoader(shadersDir + "f_sun.frag")));
		//
		//shaderLoaders.insert(std::pair("v_hud", ShaderLoader(shadersDir + "v_hud.vert")));
		//shaderLoaders.insert(std::pair("f_hud", ShaderLoader(shadersDir + "f_hud.frag")));
		//
		//shaderLoaders.insert(std::pair("v_atmosphere", ShaderLoader(shadersDir + "v_atmosphere.vert")));
		//shaderLoaders.insert(std::pair("f_atmosphere", ShaderLoader(shadersDir + "f_atmosphere.frag")));

		shaderLoaders.insert(std::pair("v_treeBB", ShaderLoader(shadersDir + "basic_v.vert", std::vector<shaderModifier>{sm_verticalNormals, sm_waving_weak})));
		shaderLoaders.insert(std::pair("f_treeBB", ShaderLoader(shadersDir + "basic_f.frag", std::vector<shaderModifier>{sm_albedo, sm_discardAlpha, sm_reduceNightLight, sm_distDithering_far})));
		
		shaderLoaders.insert(std::pair("v_trunk", ShaderLoader(shadersDir + "basic_v.vert", std::vector<shaderModifier>{sm_displace})));
		shaderLoaders.insert(std::pair("f_trunk", ShaderLoader(shadersDir + "basic_f.frag", std::vector<shaderModifier>{sm_albedo, sm_earlyDepthTest, sm_reduceNightLight})));
		
		shaderLoaders.insert(std::pair("v_branch", ShaderLoader(shadersDir + "basic_v.vert", std::vector<shaderModifier>{sm_displace, sm_verticalNormals, sm_waving_weak})));
		shaderLoaders.insert(std::pair("f_branch", ShaderLoader(shadersDir + "basic_f.frag", std::vector<shaderModifier>{sm_albedo, sm_discardAlpha, sm_reduceNightLight})));
		
		shaderLoaders.insert(std::pair("v_grass", ShaderLoader(shadersDir + "basic_v.vert", std::vector<shaderModifier>{sm_verticalNormals, sm_waving_strong})));
		shaderLoaders.insert(std::pair("f_grass", ShaderLoader(shadersDir + "basic_f.frag", std::vector<shaderModifier>{sm_albedo, sm_discardAlpha, sm_reduceNightLight, sm_distDithering_near, sm_dryColor})));
		
		shaderLoaders.insert(std::pair("v_stone", ShaderLoader(shadersDir + "basic_v.vert", std::vector<shaderModifier>{ })));
		shaderLoaders.insert(std::pair("f_stone", ShaderLoader(shadersDir + "basic_f.frag", std::vector<shaderModifier>{sm_albedo, sm_specular, sm_roughness, sm_earlyDepthTest, sm_reduceNightLight})));
		
		//shaderLoaders.insert(std::pair("v_noPP", ShaderLoader(shadersDir + "v_noPP.vert")));
		//shaderLoaders.insert(std::pair("f_noPP", ShaderLoader(shadersDir + "f_noPP.frag")));
		
		shaderLoaders.insert(std::pair("v_lightingPass", ShaderLoader(shadersDir + "lightingPass_v.vert")));
		shaderLoaders.insert(std::pair("f_lightingPass", ShaderLoader(shadersDir + "lightingPass_f.frag")));
	}

	// VERTICES
	{
		verticesLoaders.insert(std::pair("grass", VerticesLoader(vertexDir + "grass.obj")));
		verticesLoaders.insert(std::pair("plant", VerticesLoader(vertexDir + "plant.obj")));
		verticesLoaders.insert(std::pair("stone", VerticesLoader(vertexDir + "rocks/free_rock/stone.obj")));
		verticesLoaders.insert(std::pair("trunk", VerticesLoader(vertexDir + "tree/trunk.obj")));
		verticesLoaders.insert(std::pair("branches", VerticesLoader(vertexDir + "tree/branches.obj")));
		verticesLoaders.insert(std::pair("treeBB", VerticesLoader(vertexDir + "tree/treeBB.obj")));
	}

	// TEXTURES
	{
		// Special
		texInfos.insert(std::pair("cottage_d",	TextureLoader(texDir + "models/cottage/cottage_diffuse.png")));
		texInfos.insert(std::pair("room",		TextureLoader(texDir + "models/viking_room.png")));
		texInfos.insert(std::pair("squares",	TextureLoader(texDir + "squares.png")));
		texInfos.insert(std::pair("sun",		TextureLoader(texDir + "Sun/sun2_1.png")));
		texInfos.insert(std::pair("hud",		TextureLoader(texDir + "HUD/reticule_1.png")));

		// Skybox
		texInfos.insert(std::pair("sb_space1",	TextureLoader(texDir + "skybox/space1.jpg")));
		texInfos.insert(std::pair("sb_front",	TextureLoader(texDir + "skybox/blue1/front.jpg",	VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		texInfos.insert(std::pair("sb_back",	TextureLoader(texDir + "skybox/blue1/back.jpg",	VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		texInfos.insert(std::pair("sb_up",		TextureLoader(texDir + "skybox/blue1/up.jpg",	VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		texInfos.insert(std::pair("sb_down",	TextureLoader(texDir + "skybox/blue1/down.jpg",	VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		texInfos.insert(std::pair("sb_right",	TextureLoader(texDir + "skybox/blue1/right.jpg",	VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		texInfos.insert(std::pair("sb_left",	TextureLoader(texDir + "skybox/blue1/left.jpg",	VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));

		// Plants
		texInfos.insert(std::pair("grassDry_a", TextureLoader(texDir + "grass/dry/grass_a.png")));
		texInfos.insert(std::pair("grassDry_n", TextureLoader(texDir + "grass/dry/grass_n.png")));
		texInfos.insert(std::pair("grassDry_s", TextureLoader(texDir + "grass/dry/grass_s.png")));
		texInfos.insert(std::pair("grassDry_r", TextureLoader(texDir + "grass/dry/grass_r.png")));
		texInfos.insert(std::pair("grassDry_h", TextureLoader(texDir + "grass/dry/grass_h.png")));

		texInfos.insert(std::pair("bark_a", TextureLoader(vertexDir + "tree/bark_a.jpg")));
		//texInfos.insert(std::pair("bark_s", TextureLoader(vertexDir + "tree/bark_s.png")));
		texInfos.insert(std::pair("branch_a", TextureLoader(vertexDir + "tree/branch_a.png")));
		texInfos.insert(std::pair("treeBB_a", TextureLoader(vertexDir + "tree/billboards/treeBB_a.png")));

		texInfos.insert(std::pair("grass", TextureLoader(texDir + "grass/grass.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		texInfos.insert(std::pair("plant", TextureLoader(texDir + "grass/plant.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		//texInfos.insert(std::pair("grass1", TextureLoader(texDir + "grass/grass1.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		//texInfos.insert(std::pair("grass2", TextureLoader(texDir + "grass/grass2.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT)));
		texInfos.insert(std::pair("whiteNoise", TextureLoader(texDir + "grass/whiteNoise.png")));

		// Rocks
		texInfos.insert(std::pair("rocky_a", TextureLoader(texDir + "rock/cracks_2/rocky_a.png")));
		texInfos.insert(std::pair("rocky_n", TextureLoader(texDir + "rock/cracks_2/rocky_n.png")));
		texInfos.insert(std::pair("rocky_s", TextureLoader(texDir + "rock/cracks_2/rocky_s.png")));
		texInfos.insert(std::pair("rocky_r", TextureLoader(texDir + "rock/cracks_2/rocky_r.png")));
		texInfos.insert(std::pair("rocky_h", TextureLoader(texDir + "rock/cracks_2/rocky_h.png")));

		texInfos.insert(std::pair("stone_a", TextureLoader(vertexDir + "rocks/free_rock/stone_a.jpg")));
		texInfos.insert(std::pair("stone_n", TextureLoader(vertexDir + "rocks/free_rock/stone_n.jpg")));
		texInfos.insert(std::pair("stone_s", TextureLoader(vertexDir + "rocks/free_rock/stone_s.jpg")));
		texInfos.insert(std::pair("stone_r", TextureLoader(vertexDir + "rocks/free_rock/stone_r.jpg")));

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
		texInfos.insert(std::pair("sea_n", TextureLoader(texDir + "water/sea_n.png")));
		texInfos.insert(std::pair("sea_h", TextureLoader(texDir + "water/sea_h.png")));
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
	}

	// TEXTURE PACKS
	{
		// Plants
		soilTexInfos.push_back(texInfos["grassDry_a"]);
		soilTexInfos.push_back(texInfos["grassDry_n"]);
		soilTexInfos.push_back(texInfos["grassDry_s"]);
		soilTexInfos.push_back(texInfos["grassDry_r"]);
		soilTexInfos.push_back(texInfos["grassDry_h"]);
		// Rocks
		soilTexInfos.push_back(texInfos["rocky_a"]);
		soilTexInfos.push_back(texInfos["rocky_n"]);
		soilTexInfos.push_back(texInfos["rocky_s"]);
		soilTexInfos.push_back(texInfos["rocky_r"]);
		soilTexInfos.push_back(texInfos["rocky_h"]);
		// Snow
		soilTexInfos.push_back(texInfos["snow_a"]);
		soilTexInfos.push_back(texInfos["snow_n"]);
		soilTexInfos.push_back(texInfos["snow_s"]);
		soilTexInfos.push_back(texInfos["snow_r"]);
		soilTexInfos.push_back(texInfos["snow_h"]);
		soilTexInfos.push_back(texInfos["snow2_a"]);
		soilTexInfos.push_back(texInfos["snow2_n"]);
		soilTexInfos.push_back(texInfos["snow2_s"]);
		soilTexInfos.push_back(texInfos["snow_r"]);	// repeated
		soilTexInfos.push_back(texInfos["snow_h"]);	// repeated
		// Soils
		soilTexInfos.push_back(texInfos["sandDunes_a"]);
		soilTexInfos.push_back(texInfos["sandDunes_n"]);
		soilTexInfos.push_back(texInfos["sandDunes_s"]);
		soilTexInfos.push_back(texInfos["sandDunes_r"]);
		soilTexInfos.push_back(texInfos["sandDunes_h"]);
		soilTexInfos.push_back(texInfos["sandWavy_a"]);
		soilTexInfos.push_back(texInfos["sandWavy_n"]);
		soilTexInfos.push_back(texInfos["sandWavy_s"]);
		soilTexInfos.push_back(texInfos["sandWavy_r"]);
		soilTexInfos.push_back(texInfos["sandWavy_h"]);
		soilTexInfos.push_back(texInfos["squares"]);
		// Water
		soilTexInfos.push_back(texInfos["sea_n"]);
		soilTexInfos.push_back(texInfos["sea_h"]);
		soilTexInfos.push_back(texInfos["sea_foam_a"]);
	}

	{
		seaTexInfos.push_back(texInfos["sea_n"]);
		seaTexInfos.push_back(texInfos["sea_h"]);
		seaTexInfos.push_back(texInfos["sea_foam_a"]);
		seaTexInfos.push_back(texInfos["sb_space1"]);

		seaTexInfos.push_back(texInfos["sb_front"]);
		seaTexInfos.push_back(texInfos["sb_back"]);
		seaTexInfos.push_back(texInfos["sb_up"]);
		seaTexInfos.push_back(texInfos["sb_down"]);
		seaTexInfos.push_back(texInfos["sb_right"]);
		seaTexInfos.push_back(texInfos["sb_left"]);
	}

	{
		skyboxTexInfos.push_back(texInfos["sb_front"]);
		skyboxTexInfos.push_back(texInfos["sb_back"]);
		skyboxTexInfos.push_back(texInfos["sb_up"]);
		skyboxTexInfos.push_back(texInfos["sb_down"]);
		skyboxTexInfos.push_back(texInfos["sb_right"]);
		skyboxTexInfos.push_back(texInfos["sb_left"]);
	}
}