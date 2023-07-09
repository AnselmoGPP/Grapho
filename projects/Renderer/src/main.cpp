/*
	#include hierarchy:

	-toolkit
	-renderer
		>models
			-vertex
			-ubo
				>environment
				>commons
			-texture
				>environment
				>commons
			-loaddata
				>vertex
				>commons
			-commons
		>input
			-camera
				>physics
		>timer
		>commons
			-Environment	
*/


#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "toolkit.hpp"

#include "commons.hpp"


std::map<std::string, modelIter> assets;	// Model iterators
std::map<std::string, texIter> textures;	// Texture iterators
std::map<std::string, shaderIter> shaders;		// Shaders

FreePolarCam camera(
	glm::vec3(0.f, 0.f, 20.0f),			// camera position
	50.f, 0.001f, 5.f,					// keyboard/mouse/scroll speed
	60.f, 10.f, 100.f,					// FOV, minFOV, maxFOV
	glm::vec3(90.f, 0.f, 0.f),			// Yaw (z), Pitch (x), Roll (y)
	0.1f, 5000.f,						// near & far view planes
	glm::vec3(0.0f, 0.0f, 1.0f));		// world up

void update(Renderer& rend, glm::mat4 view, glm::mat4 proj);
void setPoints(Renderer& app);

int main(int argc, char* argv[])
{
	TimerSet time;
	std::cout << "------------------------------" << std::endl << time.getDate() << std::endl;

	Renderer app(update, &camera, 3);

	setPoints(app);

	app.renderLoop();		// Start rendering

	std::cout << "main() end" << std::endl;
	return EXIT_SUCCESS;
}

void update(Renderer& rend, glm::mat4 view, glm::mat4 proj)
{
	long double frameTime	= rend.getTimer().getTime();
	size_t fps				= rend.getTimer().getFPS();
	size_t maxfps			= rend.getTimer().getMaxPossibleFPS();
	glm::vec3 camPos		= rend.getCamera().camPos;

	if (assets.find("points") != assets.end())
		for (size_t i = 0; i < assets["points"]->vsDynUBO.numDynUBOs; i++) 
		{
			uint8_t* dest = assets["points"]->vsDynUBO.getUBOptr(i);
			memcpy(dest + 1 * size.mat4, &view, size.mat4);
			memcpy(dest + 2 * size.mat4, &proj, size.mat4);
		}
}

void setPoints(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;
	/*
	textures0["tex"] = app.newTexture(".../../../textures/tech_a.png");

	shaders0["v_point"] = app.newShader("../../../projects/Renderer/shaders/GLSL/v_pointPC.vert");
	shaders0["f_point"] = app.newShader("../../../projects/Renderer/shaders/GLSL/f_pointPC.frag");

	Icosahedron icos(30.f);	// Just created for calling destructor, which applies a multiplier.
	VertexType vertexType({ vec3size, vec3size }, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT });
	DataLoader* DataLoader = new DataFromUser_computed(vertexType, Icosahedron::icos.size() / 6, Icosahedron::icos.data(), noIndices);

	VerticesLoader vertexData(vt_33, Icosahedron::icos.data(), Icosahedron::icos.size() / 6, noIndices);
	std::vector<ShaderInfo> shaders{ 
		ShaderInfo(std::string("../../../projects/Renderer/shaders/GLSL/v_pointPC.vert")), 
		ShaderInfo(std::string("../../../projects/Renderer/shaders/GLSL/f_pointPC.frag")) };

	assets["points"] = app.newModel( 
		"points",
		1, 1, primitiveTopology::point,
		vertexData, shaders, noTextures,
		1, 3 * mat4size,	// M, V, P
		0,
		false);

	memcpy(assets["points"]->vsDynUBO.getUBOptr(0), &modelMatrix(), mat4size);
	*/
}