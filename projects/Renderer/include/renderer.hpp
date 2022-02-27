#ifndef RENDERER_HPP
#define RENDERER_HPP

/*
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
#include <optional>			// std::optional<uint32_t> (Wrapper that contains no value until you assign something to it. Contains member has_value())

#include "models.hpp"
#include "input.hpp"
#include "timer.hpp"

typedef std::list<ModelData>::iterator modelIterator;
typedef std::list<Texture>::iterator texIterator;

/// Used for the user to specify what primitive type represents the vertex data. 
enum primitiveTopology {
	point		= VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	line		= VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	triangle	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
};

// LOOK Restart the Renderer object after finishing the render loop
/**
*   @brief Responsible for making the rendering (render loop). Manages models, textures, input, camera...
* 
*	It creates a VulkanEnvironment and, when the user wants, a ModelData (newModel()).
*/
class Renderer
{
	VulkanEnvironment		e;
	Input					input;			//!< Input data
	TimerSet				timer;			//!< Time control
	std::list<ModelData>	models;			//!< Models (fully initialized)
	//std::list<Texture>	textures;		//!< Texture set

	// Threads stuff
	std::thread				thread_loadModels;				//!< Thread for loading new models. Initiated in the constructor. Finished if glfwWindowShouldClose

	std::mutex				mutex_modelsAndCommandBuffers;	//!< Controls access to models list and the command buffer
	std::mutex				mutex_modelsToLoad;				//!< Controls access to modelsToLoad list
	std::mutex				mutex_modelsToDelete;			//!< Controls access to modelsToDelete list
	std::mutex				mutex_rendersToSet;				//!< Controls access to rendersToSet map
	std::mutex				mutex_resizingWindow;			//!< Controls any change to Vulkan objects (for 2nd thread & resizing window)

	std::list<ModelData>				modelsToLoad;		//!< Models waiting for being included in m (partially initialized).
	std::list<modelIterator>			modelsToDelete;		//!< Iterators to the loaded models that have to be deleted from Vulkan.
	//std::list<Texture>				texturesToLoad;
	//std::list<Texture>				texturesToDelete;
	std::map<modelIterator*, size_t>	rendersToSet;		//!< Number of renderings per model.

	// Private parameters:

	const int MAX_FRAMES_IN_FLIGHT		= 2;				//!< How many frames should be processed concurrently.
	VkClearColorValue backgroundColor	= { 50/255.f, 150/255.f, 255/255.f, 1.0f };
	int maxFPS							= 80;
	int waitTime						= 500;

	// Main methods:

	/*
		@brief Allocates command buffers and record drawing commands in them.

		Render same model with different descriptors (used here):
		<ul>
			<li>You technically don't have multiple uniform buffers; you just have one. But you can use the offset(s) provided to vkCmdBindDescriptorSets to shift where in that buffer the next rendering command(s) will get their data from. Basically, you rebind your descriptor sets, but with different pDynamicOffset array values.</li>
			<li>Your pipeline layout has to explicitly declare those descriptors as being dynamic descriptors. And every time you bind the set, you'll need to provide the offset into the buffer used by that descriptor.</li>
			<li>More: https://stackoverflow.com/questions/45425603/vulkan-is-there-a-way-to-draw-multiple-objects-in-different-locations-like-in-d </li>
		</ul>

		Another option: Instance rendering allows to perform a single draw call. As I understood, UBO can be passed in the following ways:
		<ul>
			<li>In a non-dynamic descriptor (problem: shader has to contain the declaration of one UBO for each instance).</li>
			<li>As a buffer's attribute (problem: it is non-modifyable).</li>
			<li>In a dynamic descriptor (solves both previous problems), but requires many draw calls (defeats the whole purpose of instance rendering).</li>
			<li>https://stackoverflow.com/questions/54619507/whats-the-correct-way-to-implement-instanced-rendering-in-vulkan </li>
			<li>https://www.reddit.com/r/vulkan/comments/hhoktq/rendering_multiple_objects/ </li>
		</ul>
	*/
	void createCommandBuffers();

	/// Create semaphores and fences for synchronizing the events occuring in each frame (drawFrame()).
	void createSyncObjects();
	void mainLoop();

	/**
	*	Acquire image from swap chain, execute command buffer with that image as attachment in the framebuffer, and return the image to the swap chain for presentation.
	*	This method performs 3 operations asynchronously (the function call returns before the operations are finished, with undefined order of execution):
	*	<ul>
	*		<li>Acquire an image from the swap chain</li>
	*		<li>Execute the command buffer with that image as attachment in the framebuffer</li>
	*		<li>Return the image to the swap chain for presentation</li>
	*	</ul>
	*	Each of the operations depends on the previous one finishing, so we need to synchronize the swap chain events.
	*	Two ways: semaphores (mainly designed to synchronize within or accross command queues. Best fit here) and fences (mainly designed to synchronize your application itself with rendering operation).
	*	Synchronization examples: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present
	*/
	void drawFrame();

	/// Cleanup after render loop terminates
	void cleanup();

	/// Update Uniform buffer. It will generate a new transformation every frame to make the geometry spin around.
	void updateUniformBuffer(uint32_t currentImage);
	void(*userUpdate) (Renderer& rend, glm::mat4 view, glm::mat4 proj);

	/// Check for models pending full initialization.
	void loadModels_Thread();

	/// The window surface may change, making the swap chain no longer compatible with it (example: window resizing). Here, we catch these events and recreate the swap chain.
	void recreateSwapChain();

	/// Used in recreateSwapChain()
	void cleanupSwapChain();
	void stopThread();

	// Member variables:

	std::vector<VkCommandBuffer> commandBuffers;			//!< <<< List. Opaque handle to command buffer object. One for each swap chain framebuffer.

	std::vector<VkSemaphore>	imageAvailableSemaphores;	//!< Signals that an image has been acquired and is ready for rendering. Each frame has a semaphore for concurrent processing. Allows multiple frames to be in-flight while still bounding the amount of work that piles up. One for each possible frame in flight.
	std::vector<VkSemaphore>	renderFinishedSemaphores;	//!< Signals that rendering has finished and presentation can happen. Each frame has a semaphore for concurrent processing. Allows multiple frames to be in-flight while still bounding the amount of work that piles up. One for each possible frame in flight.
	std::vector<VkFence>		inFlightFences;				//!< Similar to semaphores, but fences actually wait in our own code. Used to perform CPU-GPU synchronization. One for each possible frame in flight.
	std::vector<VkFence>		imagesInFlight;				//!< Maps frames in flight by their fences. Tracks for each swap chain image if a frame in flight is currently using it. One for each swap chain image.

	size_t						currentFrame;				//!< Frame to process next (0 or 1).
	bool						runThread;					//!< Signals whether the secondary thread (thread_loadModels) should be running.

public:

	// LOOK what if firstModel.size() == 0
	/// Constructor. Requires a callback for updating model matrix, adding models, deleting models, etc.
	Renderer(void(*graphicsUpdate)(Renderer&, glm::mat4 view, glm::mat4 proj));
	~Renderer();

	/// Create command buffer and start render loop.
	int				run();

	/// Returns the timer object (provides access to time data).
	TimerSet&		getTimer();

	/// Returns the camera object (provides access to camera data).
	Camera&			getCamera();


	Input&			getInput();

	/**
		@brief Inserts a partially initialized model object.The loadModels_Thread() thread will fully initialize it as soon as possible.

		@param numberOfRenderings Number of times this model can be rendered in the same model
		@param primitiveTopology Primitive topology that the vertex data represents
		@param vsUboType Structure for the vertex shader UBO
		@param fsUboType Structure for the fragment shader UBO
		@param modelPath Path to the model (OBJ file)
		@param textures Set of textures
		@param VSpath Path to the vertex shader
		@param FSpath Path to the fragment shader
		@param vertexType
		@param transparency
	*/
	modelIterator	newModel(size_t numRenderings, primitiveTopology primitiveTopology, VertexLoader* vertexLoader, const UBOconfig& vsUboConfig, const UBOconfig& fsUboConfig, std::vector<Texture>& textures, const char* VSpath, const char* FSpath, bool transparency);
	//modelIterator	newModel(size_t numRenderings, primitiveTopology primitiveTopology, VertexLoader* vertexLoader, const UBOconfig& vsUboConfig, const UBOconfig& fsUboConfig, std::vector<Texture>& textures, const char* VSpath, const char* FSpath, bool transparency);
	void			deleteModel(modelIterator model);

	texIterator		newTexture(std::vector<Texture>& textures);
	void			deleteTexture(texIterator texture);

	void			setRenders(modelIterator& model, size_t numberOfRenders);
};

#endif