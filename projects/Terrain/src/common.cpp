
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
/*
FreePolarCam camera_1(
	glm::vec3(0.f, 0.f, 5.0f),		// camera position
	2.f, 0.001f, 0.1f,				// keyboard/mouse/scroll speed
	glm::vec3(90.f, 0.f, 0.f),		// Yaw (z), Pitch (x), Roll (y)
	glm::vec3(0.0f, 0.0f, 1.0f) );	// world up

SphereCam camera_2(
	500.f, 0.002f, 0.1f,			// keyboard/mouse/scroll speed
	glm::vec3(0.f, 0.f, 1.f),		// world up
	glm::vec3(0.f, 0.f, 0.f),		// nucleus
	4000.f,							// radius
	45.f, 45.f );					// latitude & longitude

>>> PlaneCam camera_3(
	glm::vec3(1450.f, 1450.f, 0.f),	// camera position
	100.f, 0.001f, 0.1f,			// keyboard/mouse/scroll speed
	glm::vec3(0.f, -90.f, 0.f),		// Yaw (z), Pitch (x), Roll (y)

PlanetFPcam camera_4(
	2.f, 0.001f, 0.1f,				// keyboard/mouse/scroll speed
	{ 0.f, 0.f, 0.f },				// nucleus
	1000,							// radius
	0.f, 47.f );					// latitude & longitude
*/

ShaderLoader ShaderLoaders[]
{
	/*00*/ ShaderLoader(shadersDir + "v_points.vert"),
	/*01*/ ShaderLoader(shadersDir + "f_points.frag"),

	/*02*/ ShaderLoader(shadersDir + "v_lines.vert"),
	/*03*/ ShaderLoader(shadersDir + "f_lines.frag"),

	/*04*/ ShaderLoader(shadersDir + "v_skybox.vert"),
	/*05*/ ShaderLoader(shadersDir + "f_skybox.frag"),

	/*06*/ ShaderLoader(shadersDir + "v_basicModel.vert"),
	/*07*/ ShaderLoader(shadersDir + "f_basicModel.frag"),

	/*08*/ ShaderLoader(shadersDir + "v_grass.vert"),
	/*09*/ ShaderLoader(shadersDir + "f_grass.frag"),

	/*10*/ ShaderLoader(shadersDir + "v_seaPlanet.vert"),
	/*11*/ ShaderLoader(shadersDir + "f_seaPlanet.frag"),

	/*12*/ ShaderLoader(shadersDir + "v_plainChunk.vert"),
	/*13*/ ShaderLoader(shadersDir + "f_plainChunk.frag"),

	/*14*/ ShaderLoader(shadersDir + "v_planetChunk.vert"),
	/*15*/ ShaderLoader(shadersDir + "f_planetChunk.frag"),

	/*16*/ ShaderLoader(shadersDir + "v_sun.vert"),
	/*17*/ ShaderLoader(shadersDir + "f_sun.frag"),

	/*18*/ ShaderLoader(shadersDir + "v_hud.vert"),
	/*19*/ ShaderLoader(shadersDir + "f_hud.frag"),

	/*20*/ ShaderLoader(shadersDir + "v_atmosphere.vert"),
	/*21*/ ShaderLoader(shadersDir + "f_atmosphere.frag"),

	/*22*/ ShaderLoader(shadersDir + "v_noPP.vert"),
	/*23*/ ShaderLoader(shadersDir + "f_noPP.frag"),

	/*24*/ ShaderLoader(shadersDir + "v_XXX.vert"),
	/*25*/ ShaderLoader(shadersDir + "f_XXX.frag")
};

TextureLoader texInfos[]
{
	// Special
	/*00*/ TextureLoader(texDir + "sky_box/space1.jpg"),//space1.jpg"),
	/*01*/ TextureLoader(texDir + "models/cottage/cottage_diffuse.png"),
	/*02*/ TextureLoader(texDir + "models/viking_room.png"),
	/*03*/ TextureLoader(texDir + "squares.png"),
	/*04*/ TextureLoader(texDir + "Sun/sun2_1.png"),
	/*05*/ TextureLoader(texDir + "HUD/reticule_1.png"),

	// Plants
	/*06*/ TextureLoader(texDir + "grass/grassDry_a.png"),
	/*07*/ TextureLoader(texDir + "grass/grassDry_n.png"),
	/*08*/ TextureLoader(texDir + "grass/grassDry_s.png"),
	/*09*/ TextureLoader(texDir + "grass/grassDry_r.png"),
	/*10*/ TextureLoader(texDir + "grass/grassDry_h.png"),

	// Rocks
	/*11*/ TextureLoader(texDir + "rock/bumpRock_a.png"),
	/*12*/ TextureLoader(texDir + "rock/bumpRock_n.png"),
	/*13*/ TextureLoader(texDir + "rock/bumpRock_s.png"),
	/*14*/ TextureLoader(texDir + "rock/bumpRock_r.png"),
	/*15*/ TextureLoader(texDir + "rock/bumpRock_h.png"),

	// Soils
	/*16*/ TextureLoader(texDir + "sand/sandDunes_a.png"),
	/*17*/ TextureLoader(texDir + "sand/sandDunes_n.png"),
	/*18*/ TextureLoader(texDir + "sand/sandDunes_s.png"),
	/*19*/ TextureLoader(texDir + "sand/sandDunes_r.png"),
	/*20*/ TextureLoader(texDir + "sand/sandDunes_h.png"),

	/*21*/ TextureLoader(texDir + "sand/sandWavy_a.png"),
	/*22*/ TextureLoader(texDir + "sand/sandWavy_n.png"),
	/*23*/ TextureLoader(texDir + "sand/sandWavy_s.png"),
	/*24*/ TextureLoader(texDir + "sand/sandWavy_r.png"),
	/*25*/ TextureLoader(texDir + "sand/sandWavy_h.png"),

	// Water
	/*26*/ TextureLoader(texDir + "water/sea_n.png"),
	/*27*/ TextureLoader(texDir + "water/sea_h.png"),
	/*28*/ TextureLoader(texDir + "water/sea_foam_a.png"),
	//TextureLoader(texDir + "bubbles_a.png"),

	// Snow
	/*29*/ TextureLoader(texDir + "snow/snow_a.png"),
	/*30*/ TextureLoader(texDir + "snow/snow_n.png"),
	/*31*/ TextureLoader(texDir + "snow/snow_s.png"),
	/*32*/ TextureLoader(texDir + "snow/snow_r.png"),
	/*33*/ TextureLoader(texDir + "snow/snow_h.png"),

	/*34*/ TextureLoader(texDir + "snow/snow2_a.png"),
	/*35*/ TextureLoader(texDir + "snow/snow2_n.png"),
	/*36*/ TextureLoader(texDir + "snow/snow2_s.png"),

	// Others
	/*37*/ TextureLoader(texDir + "grass/grass0.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT),
	/*38*/ TextureLoader(texDir + "grass/grass1.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT),
	/*39*/ TextureLoader(texDir + "grass/grass2.png", VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT),
	/*40*/ TextureLoader(texDir + "grass/whiteNoise.png"),
};

std::vector<TextureLoader> usedTextures
{
	// Plants
	/*00*/ TextureLoader(texDir + "grass/grassDry_a.png"),
	/*01*/ TextureLoader(texDir + "grass/grassDry_n.png"),
	/*02*/ TextureLoader(texDir + "grass/grassDry_s.png"),
	/*03*/ TextureLoader(texDir + "grass/grassDry_r.png"),
	/*04*/ TextureLoader(texDir + "grass/grassDry_h.png"),

	// Rocks
	/*05*/ TextureLoader(texDir + "rock/bumpRock_a.png"),
	/*06*/ TextureLoader(texDir + "rock/bumpRock_n.png"),
	/*07*/ TextureLoader(texDir + "rock/bumpRock_s.png"),
	/*08*/ TextureLoader(texDir + "rock/bumpRock_r.png"),
	/*09*/ TextureLoader(texDir + "rock/bumpRock_h.png"),

	// Snow
	/*10*/ TextureLoader(texDir + "snow/snow_a.png"),
	/*11*/ TextureLoader(texDir + "snow/snow_n.png"),
	/*12*/ TextureLoader(texDir + "snow/snow_s.png"),
	/*13*/ TextureLoader(texDir + "snow/snow_r.png"),
	/*14*/ TextureLoader(texDir + "snow/snow_h.png"),

	/*15*/ TextureLoader(texDir + "snow/snow2_a.png"),
	/*16*/ TextureLoader(texDir + "snow/snow2_n.png"),
	/*17*/ TextureLoader(texDir + "snow/snow2_s.png"),
	/*18*/ TextureLoader(texDir + "snow/snow_r.png"),		// repeated
	/*19*/ TextureLoader(texDir + "snow/snow_h.png"),		// repeated

	// Soils
	/*20*/ TextureLoader(texDir + "sand/sandDunes_a.png"),
	/*21*/ TextureLoader(texDir + "sand/sandDunes_n.png"),
	/*22*/ TextureLoader(texDir + "sand/sandDunes_s.png"),
	/*23*/ TextureLoader(texDir + "sand/sandDunes_r.png"),
	/*24*/ TextureLoader(texDir + "sand/sandDunes_h.png"),

	/*25*/ TextureLoader(texDir + "sand/sandWavy_a.png"),
	/*26*/ TextureLoader(texDir + "sand/sandWavy_n.png"),
	/*27*/ TextureLoader(texDir + "sand/sandWavy_s.png"),
	/*28*/ TextureLoader(texDir + "sand/sandWavy_r.png"),
	/*29*/ TextureLoader(texDir + "sand/sandWavy_h.png"),

	/*30*/ TextureLoader(texDir + "squares.png"),

	// Water
	/*31*/ TextureLoader(texDir + "water/sea_n.png"),
	/*32*/ TextureLoader(texDir + "water/sea_h.png"),
	/*33*/ TextureLoader(texDir + "water/sea_foam_a.png")
};
