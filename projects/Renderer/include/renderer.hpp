#ifndef RENDERER_HPP
#define RENDERER_HPP

/*
	Renderer creates a VulkanEnvironment and, when the user wants, a ModelData (newModel()).

	Methods:

		Thread 1:
			run()
				- createCommandBuffers (1)
				- createSyncObjects
				- mainLoop
					drawFrame
						updateUniformBuffer
							graphicsUpdate
						recreateSwapChain (for window resize)
							cleanupSwapChain
							createCommandBuffers (3)
						stopThreads
				- cleanup

		Thread 2:
			loadModels_Thread
				createCommandBuffers (2)
*/

#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <optional>				// std::optional<uint32_t> (Wrapper that contains no value until you assign something to it. Contains member has_value())

#include "environment.hpp"
#include "models.hpp"
#include "input.hpp"
#include "timer.hpp"

typedef std::list<ModelData>::iterator modelIterator;

enum primitiveTopology {
	point		= VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	line		= VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	triangle	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
};

// LOOK Restart the Renderer object after finishing the render loop
class Renderer
{
	VulkanEnvironment		e;				// Environment
	Input					input;			// Input
	TimerSet				timer;			// Time control
	std::list<ModelData>	models;			// Models (fully initialized)

	// Threads stuff
	std::thread				thread_loadModels;				// Thread for loading new models. Initiated in the constructor. Finished if glfwWindowShouldClose

	std::mutex				mutex_modelsAndCommandBuffers;	// Controls access to models list and the command buffer
	std::mutex				mutex_modelsToLoad;				// Controls access to modelsToLoad list
	std::mutex				mutex_modelsToDelete;			// Controls access to modelsToDelete list
	std::mutex				mutex_rendersToSet;				// Controls access to rendersToSet map
	std::mutex				mutex_resizingWindow;			// Controls any change to Vulkan objects (for 2nd thread & resizing window)

	std::list<ModelData>				modelsToLoad;		// Models waiting for being included in m (partially initialized).
	std::list<modelIterator>			modelsToDelete;		// Iterators to the loaded models that have to be deleted from Vulkan.
	std::map<modelIterator*, size_t>	rendersToSet;

	// Private parameters:

	const int MAX_FRAMES_IN_FLIGHT		= 2;				// How many frames should be processed concurrently.
	VkClearColorValue backgroundColor	= { 50/255.f, 150/255.f, 255/255.f, 1.0f };
	int maxFPS							= 80;
	int waitTime						= 500;

	// Main methods:

	void createCommandBuffers();	///< Allocates command buffers and record drawing commands in them.
	void createSyncObjects();
	void mainLoop();
		void drawFrame();
	void cleanup();

	void updateUniformBuffer(uint32_t currentImage);
	void(*graphicsUpdate) (Renderer& rend);
	void loadModels_Thread();

	void recreateSwapChain();
	void cleanupSwapChain();
	void stopThread();

	// Member variables:

	std::vector<VkCommandBuffer> commandBuffers;			///<<< List. Opaque handle to command buffer object. One for each swap chain framebuffer.

	std::vector<VkSemaphore>	imageAvailableSemaphores;	///< Signals that an image has been acquired and is ready for rendering. Each frame has a semaphore for concurrent processing. Allows multiple frames to be in-flight while still bounding the amount of work that piles up. One for each possible frame in flight.
	std::vector<VkSemaphore>	renderFinishedSemaphores;	///< Signals that rendering has finished and presentation can happen. Each frame has a semaphore for concurrent processing. Allows multiple frames to be in-flight while still bounding the amount of work that piles up. One for each possible frame in flight.
	std::vector<VkFence>		inFlightFences;				///< Similar to semaphores, but fences actually wait in our own code. Used to perform CPU-GPU synchronization. One for each possible frame in flight.
	std::vector<VkFence>		imagesInFlight;				///< Maps frames in flight by their fences. Tracks for each swap chain image if a frame in flight is currently using it. One for each swap chain image.

	size_t						currentFrame;				///< Frame to process next (0 or 1).
	bool						runThread;					///< Signals whether the secondary thread (thread_loadModels) should be running.

public:
	Renderer(void(*graphicsUpdate)(Renderer&));	// LOOK what if firstModel.size() == 0
	~Renderer();

	int				run();

	TimerSet&		getTimer();
	Camera&			getCamera();

	modelIterator	newModel(size_t numberOfRenderings, const char* modelPath, const char* texturePath, const char* VSpath, const char* FSpath, VertexType vertexType, primitiveTopology primitiveTopology = primitiveTopology::triangle, bool transparency = false);
	modelIterator	newModel(size_t numberOfRenderings, const VertexType& vertexType, size_t numVertex, const void* vertexData, std::vector<uint32_t>* indices, const char* texturePath, const char* VSpath, const char* FSpath, primitiveTopology primitiveTopology = primitiveTopology::triangle, bool transparency = false);

	void			deleteModel(modelIterator model);
	void			setRenders(modelIterator& model, size_t numberOfRenders);
};

#endif