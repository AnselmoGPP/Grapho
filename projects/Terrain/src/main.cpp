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
//void setLights();
//float getFloorHeight(const glm::vec3& pos);
//float getSeaHeight(const glm::vec3& pos);
void tests();

// Models, textures, & shaders
std::map<std::string, modelIter> assets;	// Model iterators

EntityManager world;

// main ---------------------------------------------------------------------

int main(int argc, char* argv[])
{
	#ifdef DEBUG_MAIN
		//std::cout << std::setprecision(7);
		std::cout << "Current path: " << std::filesystem::current_path() << std::endl;
		TimerSet time;
		std::cout << "--------------------" << std::endl << time.getDate() << std::endl;
	#endif

	//tests(); return 0;

	try   // https://www.tutorialspoint.com/cplusplus/cpp_exceptions_handling.htm
	{
		IOmanager io(1920/2, 1080/2);
		Renderer app(update, io, 2);				// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
		EntityFactory eFact(app);
		
		// ENTITIES + COMPONENTS:

		world.addEntity(std::vector<Component*>{	// Singleton components.
			new c_Engine(app),
			new c_Input,
			new c_Camera(1),
			new c_Sky(0.0035, 0, 0.0035+0.00028, 0, 40),
			new c_Lights(3) });
		world.addEntity(eFact.createSphere(ShaderLoaders[10], ShaderLoaders[11], usedTextures));
		world.addEntity(eFact.createSkyBox(ShaderLoaders[4], ShaderLoaders[5], { texInfos[0] }));
		world.addEntity(eFact.createSun(ShaderLoaders[16], ShaderLoaders[17], { texInfos[4] }));
		world.addEntity(eFact.createAxes(ShaderLoaders[2], ShaderLoaders[3], { }));
		world.addEntity(eFact.createGrid(ShaderLoaders[2], ShaderLoaders[3], { }));
		world.addEntity(eFact.createPoints(ShaderLoaders[0], ShaderLoaders[1], { }));	// <<<
		world.addEntity(eFact.createNoPP(ShaderLoaders[22], ShaderLoaders[23], { texInfos[4], texInfos[5] }));
		
		// SYSTEMS:

		world.addSystem(new s_Engine);
		world.addSystem(new s_Input);
		world.addSystem(new s_SphereCam);	// s_SphereCam, s_PolarCam, s_PlaneCam, s_FPCam
		world.addSystem(new s_Sky_XY);		// s_Sky_XY, s_Sky_XZ
		world.addSystem(new s_Lights);
		world.addSystem(new s_Move);
		world.addSystem(new s_ModelMatrix);
		world.addSystem(new s_Model);		// update UBOs
		
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

	world.update(rend.getTimer().getDeltaTime());
}

void tests()
{

}