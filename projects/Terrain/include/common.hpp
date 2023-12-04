#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

#include "ubo.hpp"
#include "importer.hpp"
#include "ECSarch.hpp"

// MVS executable's path        == Grapho\_BUILD\projects\Terrain (Terrain.sln)
// Standalone executable's path == Grapho\_BUILD\projects\Terrain\Release (Terrain.exe)
#define STANDALONE_EXECUTABLE false

// File's paths
extern const std::string shadersDir;
extern const std::string vertexDir;
extern const std::string texDir;

// Camera
//extern FreePolarCam camera_1;
//extern SphereCam camera_2;
//extern PlaneCam camera_3;
//extern PlanetFPcam camera_4;

// Resources

extern ShaderLoader ShaderLoadersX[];
extern TextureLoader texInfosX[];

extern std::vector<TextureLoader> usedTexturesX;	// Package of textures

#endif
