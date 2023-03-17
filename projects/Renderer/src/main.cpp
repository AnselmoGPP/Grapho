#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "toolkit.hpp"

#include "commons.hpp"


std::map<std::string, modelIterator> assets;	// Model iterators
std::map<std::string, texIterator> textures;	// Texture iterators
std::map<std::string, ShaderIter> shaders;		// Shaders

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

	app.run();		// Start rendering

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
			memcpy(dest + 1 * mat4size, &view, mat4size);
			memcpy(dest + 2 * mat4size, &proj, mat4size);
		}
}

void setPoints(Renderer& app)
{
	std::cout << "> " << __func__ << "()" << std::endl;

	textures["tex"] = app.newTexture(".../../../textures/tech_a.png");

	shaders["v_point"] = app.newShader("../../../projects/Renderer/shaders/GLSL/v_pointPC.vert", shaderc_vertex_shader, true);
	shaders["f_point"] = app.newShader("../../../projects/Renderer/shaders/GLSL/f_pointPC.frag", shaderc_fragment_shader, true);

	Icosahedron icos(30.f);	// Just created for calling destructor, which applies a multiplier.
	VertexLoader* vertexLoader = new VertexFromUser(VertexType(1, 1, 0, 0), Icosahedron::icos.size() / 6, Icosahedron::icos.data(), noIndices, false);

	assets["points"] = app.newModel( 
		"points",
		1, 1, primitiveTopology::point,
		vertexLoader,
		1, 3 * mat4size,	// M, V, P
		0,
		noTextures,
		shaders["v_point"], shaders["f_point"],
		false);

	memcpy(assets["points"]->vsDynUBO.getUBOptr(0), &modelMatrix(), mat4size);
}