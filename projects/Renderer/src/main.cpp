/*
	loopManager < VulkanEnvironment
				< modelData		< VulkanEnvironment
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

	Rendering:
		- Many models
		- Same model many times
		> Add new model at render time (or delete it): New model or and already loaded one. > New thread?
		Points, lines, triangles
		Transparencies
		2D graphics
		Draw in front of some rendering (used for weapons)
		Shading stuff (lights, diffuse, ...)

		One model, many renders. Operations:
				- Add/delete/block model/s
				- Add/delete/block render/s
		
		Completed
			X Parallel loading
			  Many threads
			X Add model (after & during rendering)
			X Remove model (after & during rendering)
			  Add render (after & during rendering)
			  Remove render (after & during rendering)
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
#include <map>

#include "renderer.hpp"
#include "models.hpp"
#include "data.hpp"

void update(Renderer& r);

std::map<std::string, std::list<modelData>::iterator> assets;
bool roomVisible = false;
bool cottageLoaded = false;

int main(int argc, char* argv[])
{
	Renderer app(update);

	std::list<modelData>::iterator itCottage = app.newModel( 1,
		(MODELS_DIR   + "cottage_obj.obj").c_str(),
		(TEXTURES_DIR + "cottage/cottage_diffuse.png").c_str(),
		(SHADERS_DIR  + "triangleV.spv").c_str(),
		(SHADERS_DIR  + "triangleF.spv").c_str() );

	cottageLoaded = true;
	assets.insert({ "cottage", itCottage });

	app.run();

	//std::this_thread::sleep_for(std::chrono::seconds(5));
	return EXIT_SUCCESS;
}

void update(Renderer &r)
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
		std::list<modelData>::iterator itRoom = r.newModel(4,
			(MODELS_DIR + "viking_room.obj").c_str(),
			(TEXTURES_DIR + "viking_room.png").c_str(),
			(SHADERS_DIR + "triangleV.spv").c_str(),
			(SHADERS_DIR + "triangleF.spv").c_str());

		assets.insert({ "room", itRoom });
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