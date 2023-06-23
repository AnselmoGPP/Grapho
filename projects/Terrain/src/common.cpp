
#include "common.hpp"

#include <glm/glm.hpp>


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


// Cameras --------------------------------------------------

FreePolarCam camera_1(
	glm::vec3(0.f, 0.f, 20.0f),		// camera position
	50.f, 0.001f, 0.1f,				// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,				// FOV, minFOV, maxFOV
	glm::vec3(90.f, 0.f, 0.f),		// Yaw (z), Pitch (x), Roll (y)
	0.2f, 4000.f,					// near & far view planes
	glm::vec3(0.0f, 0.0f, 1.0f) );	// world up

SphereCam camera_2(
	100.f, 0.002f, 0.1f,			// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,				// FOV, minFOV, maxFOV
	0.2f, 4000,						// near & far view planes
	glm::vec3(0.f, 0.f, 1.f),		// world up
	glm::vec3(0.f, 0.f, 0.f),		// nucleus
	4000.f,							// radius
	45.f, 45.f );					// latitude & longitude

PlaneCam camera_3(
	glm::vec3(0.f, 2050.f, 0.f),	// camera position
	50.f, 0.001f, 0.1f,				// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,				// FOV, minFOV, maxFOV
	glm::vec3(0.f, -90.f, 0.f),		// Yaw (z), Pitch (x), Roll (y)
	0.2f, 4000.f );					// near & far view planes

PlanetFPcam camera_4(
	10.f, 0.001f, 0.1f,				// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,				// FOV, minFOV, maxFOV
	0.2f, 4000,						// near & far view planes
	{ 0.f, 0.f, 0.f },				// nucleus
	1000,							// radius
	0.f, 90.f );					// latitude & longitude

ShaderInfo shaderInfos[]
{
	/*00*/ ShaderInfo(shadersDir + "v_pointPC.vert"),
	/*01*/ ShaderInfo(shadersDir + "f_pointPC.frag"),

	/*02*/ ShaderInfo(shadersDir + "v_linePC.vert"),
	/*03*/ ShaderInfo(shadersDir + "f_linePC.frag"),

	/*04*/ ShaderInfo(shadersDir + "v_trianglePT.vert"),
	/*05*/ ShaderInfo(shadersDir + "f_trianglePT.frag"),

	/*06*/ ShaderInfo(shadersDir + "v_trianglePCT.vert"),
	/*07*/ ShaderInfo(shadersDir + "f_trianglePCT.frag"),

	/*08*/ ShaderInfo(shadersDir + "v_sea.vert"),
	/*09*/ ShaderInfo(shadersDir + "f_sea.frag"),

	/*10*/ ShaderInfo(shadersDir + "v_seaPlanet.vert"),
	/*11*/ ShaderInfo(shadersDir + "f_seaPlanet.frag"),

	/*12*/ ShaderInfo(shadersDir + "v_terrainPTN.vert"),
	/*13*/ ShaderInfo(shadersDir + "f_terrainPTN.frag"),

	/*14*/ ShaderInfo(shadersDir + "v_planetPTN.vert"),
	/*15*/ ShaderInfo(shadersDir + "f_planetPTN.frag"),

	/*16*/ ShaderInfo(shadersDir + "v_sunPT.vert"),
	/*17*/ ShaderInfo(shadersDir + "f_sunPT.frag"),

	/*18*/ ShaderInfo(shadersDir + "v_hudPT.vert"),
	/*19*/ ShaderInfo(shadersDir + "f_hudPT.frag"),

	/*20*/ ShaderInfo(shadersDir + "v_atmosphere.vert"),
	/*21*/ ShaderInfo(shadersDir + "f_atmosphere.frag"),

	/*22*/ ShaderInfo(shadersDir + "v_noPP.vert"),
	/*23*/ ShaderInfo(shadersDir + "f_noPP.frag"),

	/*24*/ ShaderInfo(shadersDir + "v_linePC_PP.vert"),
	/*25*/ ShaderInfo(shadersDir + "f_linePC_PP.frag"),
};

TextureInfo texInfos[]
{
	// Special
	/*00*/ TextureInfo(texDir + "sky_box/space1.jpg"),
	/*01*/ TextureInfo(texDir + "models/cottage/cottage_diffuse.png"),
	/*02*/ TextureInfo(texDir + "models/viking_room.png"),
	/*03*/ TextureInfo(texDir + "squares.png"),
	/*04*/ TextureInfo(texDir + "Sun/sun2_1.png"),
	/*05*/ TextureInfo(texDir + "HUD/reticule_1.png"),

	// Plants
	/*06*/ TextureInfo(texDir + "grass/grassDry_a.png"),
	/*07*/ TextureInfo(texDir + "grass/grassDry_n.png"),
	/*08*/ TextureInfo(texDir + "grass/grassDry_s.png"),
	/*09*/ TextureInfo(texDir + "grass/grassDry_r.png"),
	/*10*/ TextureInfo(texDir + "grass/grassDry_h.png"),

	// Rocks
	/*11*/ TextureInfo(texDir + "rock/bumpRock_a.png"),
	/*12*/ TextureInfo(texDir + "rock/bumpRock_n.png"),
	/*13*/ TextureInfo(texDir + "rock/bumpRock_s.png"),
	/*14*/ TextureInfo(texDir + "rock/bumpRock_r.png"),
	/*15*/ TextureInfo(texDir + "rock/bumpRock_h.png"),

	// Soils
	/*16*/ TextureInfo(texDir + "sand/sandDunes_a.png"),
	/*17*/ TextureInfo(texDir + "sand/sandDunes_n.png"),
	/*18*/ TextureInfo(texDir + "sand/sandDunes_s.png"),
	/*19*/ TextureInfo(texDir + "sand/sandDunes_r.png"),
	/*20*/ TextureInfo(texDir + "sand/sandDunes_h.png"),

	/*21*/ TextureInfo(texDir + "sand/sandWavy_a.png"),
	/*22*/ TextureInfo(texDir + "sand/sandWavy_n.png"),
	/*23*/ TextureInfo(texDir + "sand/sandWavy_s.png"),
	/*24*/ TextureInfo(texDir + "sand/sandWavy_r.png"),
	/*25*/ TextureInfo(texDir + "sand/sandWavy_h.png"),

	// Water
	/*26*/ TextureInfo(texDir + "water/sea_n.png"),
	/*27*/ TextureInfo(texDir + "water/sea_h.png"),
	/*28*/ TextureInfo(texDir + "water/sea_foam_a.png"),
	//TextureInfo(texDir + "bubbles_a.png"),

	/*29*/ TextureInfo(texDir + "snow/snow_a.png"),
	/*30*/ TextureInfo(texDir + "snow/snow_n.png"),
	/*31*/ TextureInfo(texDir + "snow/snow_s.png"),
	/*32*/ TextureInfo(texDir + "snow/snow_r.png"),
	/*33*/ TextureInfo(texDir + "snow/snow_h.png"),

	/*34*/ TextureInfo(texDir + "snow/snow2_a.png"),
	/*35*/ TextureInfo(texDir + "snow/snow2_n.png"),
	/*36*/ TextureInfo(texDir + "snow/snow2_s.png"),
};

std::vector<TextureInfo> usedTextures
{
	// Plants
	/*00*/ TextureInfo(texDir + "grass/grassDry_a.png"),
	/*01*/ TextureInfo(texDir + "grass/grassDry_n.png"),
	/*02*/ TextureInfo(texDir + "grass/grassDry_s.png"),
	/*03*/ TextureInfo(texDir + "grass/grassDry_r.png"),
	/*04*/ TextureInfo(texDir + "grass/grassDry_h.png"),

	// Rocks
	/*11*/ TextureInfo(texDir + "rock/bumpRock_a.png"),
	/*12*/ TextureInfo(texDir + "rock/bumpRock_n.png"),
	/*13*/ TextureInfo(texDir + "rock/bumpRock_s.png"),
	/*14*/ TextureInfo(texDir + "rock/bumpRock_r.png"),
	/*15*/ TextureInfo(texDir + "rock/bumpRock_h.png"),

	// Snow
	/*29*/ TextureInfo(texDir + "snow/snow_a.png"),
	/*30*/ TextureInfo(texDir + "snow/snow_n.png"),
	/*31*/ TextureInfo(texDir + "snow/snow_s.png"),
	/*32*/ TextureInfo(texDir + "snow/snow_r.png"),
	/*33*/ TextureInfo(texDir + "snow/snow_h.png"),

	/*34*/ TextureInfo(texDir + "snow/snow2_a.png"),
	/*35*/ TextureInfo(texDir + "snow/snow2_n.png"),
	/*36*/ TextureInfo(texDir + "snow/snow2_s.png"),
	/*32*/ TextureInfo(texDir + "snow/snow_r.png"),		// repeated
	/*33*/ TextureInfo(texDir + "snow/snow_h.png"),		// repeated

	// Soils
	/*16*/ TextureInfo(texDir + "sand/sandDunes_a.png"),
	/*17*/ TextureInfo(texDir + "sand/sandDunes_n.png"),
	/*18*/ TextureInfo(texDir + "sand/sandDunes_s.png"),
	/*19*/ TextureInfo(texDir + "sand/sandDunes_r.png"),
	/*20*/ TextureInfo(texDir + "sand/sandDunes_h.png"),

	/*21*/ TextureInfo(texDir + "sand/sandWavy_a.png"),
	/*22*/ TextureInfo(texDir + "sand/sandWavy_n.png"),
	/*23*/ TextureInfo(texDir + "sand/sandWavy_s.png"),
	/*24*/ TextureInfo(texDir + "sand/sandWavy_r.png"),
	/*25*/ TextureInfo(texDir + "sand/sandWavy_h.png"),

	/*03*/ TextureInfo(texDir + "squares.png"),

	// Water
	/*26*/ TextureInfo(texDir + "water/sea_n.png"),
	/*27*/ TextureInfo(texDir + "water/sea_h.png"),
	/*28*/ TextureInfo(texDir + "water/sea_foam_a.png")
};