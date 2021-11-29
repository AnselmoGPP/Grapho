/*
	Renderer	< VulkanEnvironment
				< ModelData		< VulkanEnvironment
								< modelConfig	< VulkanEnvironment
												< getModelMatrix callbacks
								< Input			< Camera

	set of modelConfig (callbacks + paths) > Renderer > set of ModelData
*/

/*
	TODO:
		> Axis
		> Sun billboard (transparencies)
		> Terrain
		Make the renderer a static library
		Add ProcessInput() maybe
		Dynamic states (graphics pipeline)
		Push constants
		Deferred rendering (https://gamedevelopment.tutsplus.com/articles/forward-rendering-vs-deferred-rendering--gamedev-12342)
		Move createDescriptorSetLayout() to Environment
		Why is FPS limited by default?

		UBO of each renders should be stored in a vector-like structure, so there are UBO available for new renders (generated with setRender())
		Destroy Vulkan buffers (UBO) outside semaphores

	Rendering:
		Points, lines, triangles
		2D graphics
		Transparencies
		Draw in front of some rendering (used for weapons)
		Shading stuff (lights, diffuse, ...)
		Make classes more secure (hide sensitive variables)
		Parallel loading (many threads)

		Allow to update MM from the beginning, so it is not required to do so repeatedly in the callback
		Updating UBOs just after modifying the amount of them with setRenders()
		Can we take stuff out from thread 2?
		- Only dynamic UBOs
		> Start thread since run() (objectAlreadyConstructed)
		Try applying alignment just to the entire UBO buffer (not individual dynamic buffers)
		Improve modelData object destruction (call stuff from destructor, and take code out from Renderer)

	BUGS:
		addRender from 2 to 1 gives a problem: the dynamic UBO requires a dynamic offset (but 0 are left)
		Somehow, camera continued moving backwards/left-side indefinetely
*/

/*
	Render same model with different descriptors
		- You technically don't have multiple uniform buffers; you just have one. But you can use the offset(s) provided to vkCmdBindDescriptorSets
		to shift where in that buffer the next rendering command(s) will get their data from. Basically, you rebind your descriptor sets, but with
		different pDynamicOffset array values.
		- Your pipeline layout has to explicitly declare those descriptors as being dynamic descriptors. And every time you bind the set, you'll need
		to provide the offset into the buffer used by that descriptor.
		- More: https://stackoverflow.com/questions/45425603/vulkan-is-there-a-way-to-draw-multiple-objects-in-different-locations-like-in-d
*/

#include <iostream>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <iomanip>
#include <map>

#include "renderer.hpp"
#include "models.hpp"
#include "data.hpp"

void update(Renderer& r);

std::map<std::string, modelIterator> assets;
bool roomVisible = false;
bool cottageLoaded = false;
bool check1 = false, check2 = false;

int main(int argc, char* argv[])
{
	std::cout << "Begin" << std::endl;
	// Create a renderer object. Pass a callback that will be called for each frame (useful for updating model view matrices).
	Renderer app(update);
	std::cout << "\n-----------------------------------------------------------------------------" << std::endl;
/*
	// Add a model to render. An iterator is returned (modelIterator). Save it for updating model data later.
	assets["cottage"] = app.newModel( 0,
		(MODELS_DIR   + "cottage_obj.obj").c_str(),
		(TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str(),
		(SHADERS_DIR  + "triangleV.spv").c_str(),
		(SHADERS_DIR  + "triangleF.spv").c_str() );

	cottageLoaded = true;

	// Delete a model you passed previously.
	app.deleteModel(assets["cottage"]);

	// Add the same model again.
	assets["cottage"] = app.newModel( 1,
		(MODELS_DIR + "cottage_obj.obj").c_str(),
		(TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str(),
		(SHADERS_DIR + "triangleV.spv").c_str(),
		(SHADERS_DIR + "triangleF.spv").c_str());

	cottageLoaded = true;
*/
	assets["room"] = app.newModel( 0,
		(MODELS_DIR + "viking_room.obj").c_str(),
		(TEXTURES_DIR + "viking_room.png").c_str(),
		(SHADERS_DIR + "triangleV.spv").c_str(),
		(SHADERS_DIR + "triangleF.spv").c_str());

	roomVisible = true;

	assets["room"]->setUBO(0, room1_MM(0));
	assets["room"]->setUBO(1, room2_MM(0));
	assets["room"]->setUBO(2, room3_MM(0));
	//assets["room"]->setUBO(3, room4_MM(0));
	//assets["room"]->setUBO(4, room5_MM(0));
/*
	assets["floor"] = app.newModel(1,
		v_floor,
		i_floor,
		(TEXTURES_DIR + "grass.png").c_str(),
		(SHADERS_DIR + "triangleV.spv").c_str(),
		(SHADERS_DIR + "triangleF.spv").c_str());
*/
	// Start rendering
	app.run();
	
	//std::this_thread::sleep_for(std::chrono::seconds(5));
	return EXIT_SUCCESS;
}

void outputData(Renderer& r)
{
	long double time = r.getTimer().getTime();
	size_t fps = r.getTimer().getFPS();
	size_t maxfps = r.getTimer().getMaxPossibleFPS();

	//std::cout << std::fixed << std::setprecision(3);
	//std::cout << fps << " - " << maxfps << " - " << time << std::endl;
	//std::cout << '\r' << fps << " - " << time;
	//std::cout.unsetf(std::ios::fixed | std::ios::scientific);
}

// Update model's model matrix each frame
void update(Renderer& r)
{
	long double time = r.getTimer().getTime();
	outputData(r);

	if (cottageLoaded)
		assets["cottage"]->setUBO(0, cottage_MM(time));

	if (time > 5 && cottageLoaded)
	{
		r.setRenders(assets["cottage"], 0);
		cottageLoaded = false;
	}

	 if (time > 5 && !check1)
	{
		std::cout << ">>> Render 4 rooms" << std::endl;
		r.setRenders(assets["room"], 4);
		check1 = true;
	}
	else if (time > 10 && !check2)
	{
		//r.setRenders(assets["room"], 3);
		//check2 = true;
	}
	
	 if (time > 5)
	 {
		 assets["room"]->setUBO(0, room1_MM(0));
		 assets["room"]->setUBO(1, room2_MM(0));
		 assets["room"]->setUBO(2, room3_MM(0));
		 assets["room"]->setUBO(3, room4_MM(0));
		 //assets["room"]->setUBO(4, room5_MM(0));
	 }
		 



}
/*
void update2(Renderer& r)
{
	long double time = r.getTimer().getTime();
	size_t fps = r.getTimer().getFPS();
	std::cout << fps << " - " << time << std::endl;

	// Update model's model matrix each frame
	if (cottageLoaded)
		assets["cottage"]->MM[0] = cottage_MM(time);

	if (time < 5)	// LOOK when setRenders(), the first frame uses default MMs
	{
		assets["room"]->MM[0] = room1_MM(time);
		assets["room"]->MM[1] = room2_MM(time);
		assets["room"]->MM[2] = room3_MM(time);
		assets["room"]->MM[3] = room4_MM(time);
	}
	else if (time > 5 && !check1)
	{
		r.setRenders(&assets["room"], 2);
		check1 = true;
	}
	else if(time < 10)
	{
		assets["room"]->MM[0] = room1_MM(time);
		assets["room"]->MM[1] = room2_MM(time);
	}
	else if (time > 10 && !check2)
	{
		r.setRenders(&assets["room"], 3);
		check2 = true;
	}
	else
	{
		assets["room"]->MM[0] = room1_MM(time);
		assets["room"]->MM[1] = room2_MM(time);
		assets["room"]->MM[2] = room3_MM(time);
	}
}

void update1(Renderer &r)
{
	long double time = r.getTimer().getTime();
	size_t fps		 = r.getTimer().getFPS();
	std::cout << fps << " - " << time << std::endl;

	// Model loaded before run(), and deleted after run()
	if (time < 5 && cottageLoaded)
		assets["cottage"]->MM[0] = cottage_MM(time);
	else if (cottageLoaded)
	{
		r.deleteModel(assets["cottage"]);
		cottageLoaded = false;
	}

	// Model loaded after run()
	if (time > 10 && !roomVisible)
	{
		assets["room"] = r.newModel(4,
			(MODELS_DIR + "viking_room.obj").c_str(),
			(TEXTURES_DIR + "viking_room.png").c_str(),
			(SHADERS_DIR + "triangleV.spv").c_str(),
			(SHADERS_DIR + "triangleF.spv").c_str());

		roomVisible = true;
	}
	else if (roomVisible)
	{
		assets["room"]->MM[0] = room1_MM(time);
		assets["room"]->MM[1] = room2_MM(time);
		assets["room"]->MM[2] = room3_MM(time);
		assets["room"]->MM[3] = room4_MM(time);
	}
}
*/