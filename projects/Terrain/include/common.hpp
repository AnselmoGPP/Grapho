#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

#include "ubo.hpp"
#include "camera.hpp"

// MVS executable's path        == Grapho\_BUILD\projects\Terrain (Terrain.sln)
// Standalone executable's path == Grapho\_BUILD\projects\Terrain\Release (Terrain.exe)
#define STANDALONE_EXECUTABLE false

// File's paths
extern const std::string shadersDir;
extern const std::string vertexDir;
extern const std::string texDir;

// Camera
extern FreePolarCam camera_1;
extern SphereCam camera_2;
extern PlaneCam camera_3;
extern PlanetFPcam camera_4;

// Texture paths
/*
std::string texPaths[]
{
	(shadersDir + "v_pointPC.vert").c_str(),
	(shadersDir + "f_pointPC.frag").c_str(),

	(shadersDir + "v_linePC.vert").c_str(),
	(shadersDir + "f_linePC.frag").c_str(),

	(shadersDir + "v_trianglePT.vert").c_str(),
	(shadersDir + "f_trianglePT.frag").c_str(),

	(shadersDir + "v_trianglePCT.vert").c_str(),
	(shadersDir + "f_trianglePCT.frag").c_str(),

	(shadersDir + "v_sea.vert").c_str(),
	(shadersDir + "f_sea.frag").c_str(),

	(shadersDir + "v_seaPlanet.vert").c_str(),
	(shadersDir + "f_seaPlanet.frag").c_str(),

	(shadersDir + "v_terrainPTN.vert").c_str(),
	(shadersDir + "f_terrainPTN.frag").c_str(),

	(shadersDir + "v_planetPTN.vert").c_str(),
	(shadersDir + "f_planetPTN.frag").c_str(),

	(shadersDir + "v_sunPT.vert").c_str(),
	(shadersDir + "f_sunPT.frag").c_str(),

	(shadersDir + "v_hudPT.vert").c_str(),
	(shadersDir + "f_hudPT.frag").c_str(),

	(shadersDir + "v_atmosphere.vert").c_str(),
	(shadersDir + "f_atmosphere.frag").c_str()
}
*/
#endif


