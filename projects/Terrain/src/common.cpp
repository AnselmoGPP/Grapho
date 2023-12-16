
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
