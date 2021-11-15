#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

/*
	Renderer creates a VulkanEnvironment and, when the user wants, a modelData (newModel()).

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
							cleanupSwapChain (2)
							createCommandBuffers (3)
						stopThreads
				- cleanup
					cleanupSwapChain (1)
					cleanupLists

		Thread 2:
			loadModels_Thread
				createCommandBuffers (2)
*/


#include <vector>
#include <thread>
#include <mutex>
#include <optional>				// std::optional<uint32_t> (Wrapper that contains no value until you assign something to it. Contains member has_value())

#include "environment.hpp"
#include "models.hpp"
#include "input.hpp"
#include "timer.hpp"

// LOOK Restart the Renderer object after finishing the render loop
class Renderer
{
	VulkanEnvironment		e;				// Environment
	Input					input;			// Input
	TimerSet				timer;			// Time control
	std::list<modelData>	models;			// Models (completly initialized)

	// Threads stuff
	std::thread				loadModelsThread;	// Thread for loading new models. Initiated in the constructor. Finished if glfwWindowShouldClose

	std::list<std::list<modelData>::iterator> deletingModels;	// Iterators to the loaded models that have to be deleted from Vulkan.
	std::list<modelData>	waitingModels;		// Models waiting for being included in m (partially initialized).

	std::mutex				modelsMutex;		// Controls access to models list and the command buffer
	std::mutex				waitingModelsMutex;	// Controls access to waitingModels list
	std::mutex				deletingModelsMutex;// Controls access to deletingModels list

	// Private parameters:

	const int MAX_FRAMES_IN_FLIGHT		= 2;										// How many frames should be processed concurrently.
	VkClearColorValue backgroundColor	= { 50/255.f, 150/255.f, 255/255.f, 1.0f };
	int maxFPS							= 80;

	// Main methods:

	void createCommandBuffers(bool justUpdate = false);	///< Allocates command buffers and record drawing commands in them. justUpdate is for using this method after loading new models in the secondary thread (avoids an initial semaphore that could block the thread).
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
	void cleanupLists();

	// Member variables:

	std::vector<VkCommandBuffer> commandBuffers;			///<<< List. Opaque handle to command buffer object. One for each swap chain framebuffer.

	std::vector<VkSemaphore>	imageAvailableSemaphores;	///< Signals that an image has been acquired and is ready for rendering. Each frame has a semaphore for concurrent processing. Allows multiple frames to be in-flight while still bounding the amount of work that piles up. One for each possible frame in flight.
	std::vector<VkSemaphore>	renderFinishedSemaphores;	///< Signals that rendering has finished and presentation can happen. Each frame has a semaphore for concurrent processing. Allows multiple frames to be in-flight while still bounding the amount of work that piles up. One for each possible frame in flight.
	std::vector<VkFence>		inFlightFences;				///< Similar to semaphores, but fences actually wait in our own code. Used to perform CPU-GPU synchronization. One for each possible frame in flight.
	std::vector<VkFence>		imagesInFlight;				///< Maps frames in flight by their fences. Tracks for each swap chain image if a frame in flight is currently using it. One for each swap chain image.

	size_t						currentFrame;				///< Frame to process next (0 or 1).
	bool						runLoadModelsThread;		///< Signals whether the secondary thread (loadModelsThread) should be running.

public:
	Renderer(void(*graphicsUpdate)(Renderer&));	// LOOK what if firstModel.size() == 0
	~Renderer();

	int run();
	std::list<modelData>::iterator newModel(size_t numberOfRenderings, const char* modelPath, const char* texturePath, const char* VSpath, const char* FSpath);
	void deleteModel(std::list<modelData>::iterator model);
	TimerSet& getTimer();
	Camera& getCamera();
};

#endif