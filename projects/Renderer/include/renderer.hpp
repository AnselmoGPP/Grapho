#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <optional>			// std::optional<uint32_t> (Wrapper that contains no value until you assign something to it. Contains member has_value())

#include "shaderc/shaderc.hpp"		// Compile GLSL code to SPIR-V

#include "models.hpp"
#include "input.hpp"
#include "timer.hpp"
#include "commons.hpp"


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
	// Hardcoded parameters
	const int MAX_FRAMES_IN_FLIGHT = 2;		//!< How many frames should be processed concurrently.
	VkClearColorValue backgroundColor = { 50 / 255.f, 150 / 255.f, 255 / 255.f, 1.0f };
	int maxFPS = 60;
	int waitTime = 500;		//!< Time the loading-thread wait till next check.

	// Main parameters
	VulkanEnvironment			e;
	Input						input;						//!< Input data
	TimerSet					timer;						//!< Time control
	std::list<ModelData>		models[2];					//!< Sets of fully initialized models (one set per render pass) [0] used to render the main colors or [1] for post processing.
	std::list<Texture>			textures;					//!< Set of textures
	std::list<VkShaderModule>	shaders;					//!< Set of shaders
	size_t						numLayers;					//!< Number of layers (Painter's algorithm)
	std::vector<modelIterator>	lastModelsToDraw;			//!< Models that must be moved to the last position in "models" in order to make them be drawn the last.

	// Threads stuff
	std::thread					thread_loadModels;			//!< Thread for loading new models. Initiated in the constructor. Finished if glfwWindowShouldClose
	std::mutex					mutSnapshot;				//!< Used for safely making a snapshot in the loading thread of the lists texturesToLoad, modelsToLoad, modelsToDelete, and texturesToDelete.

	std::list<ModelData>		modelsToLoad;				//!< Models waiting for being included in m (partially initialized).
	std::list<ModelData>		modelsToDelete;				//!< Iterators to the loaded models that have to be deleted from Vulkan.
	std::list<Texture>			texturesToLoad;				//!< Textures waiting for being loaded and moved to textures list.
	std::list<Texture>			texturesToDelete;			//!< Textures waiting for being deleted.

	// Member variables:
	std::vector<VkCommandBuffer> commandBuffers;			//!< <<< List. Opaque handle to command buffer object. One for each swap chain framebuffer.
	bool updateCommandBuffer;

	std::vector<VkSemaphore>	imageAvailableSemaphores;	//!< Signals that an image has been acquired from the swap chain and is ready for rendering. Each frame has a semaphore for concurrent processing. Allows multiple frames to be in-flight while still bounding the amount of work that piles up. One for each possible frame in flight.
	std::vector<VkSemaphore>	renderFinishedSemaphores;	//!< Signals that rendering has finished (CB has been executed) and presentation can happen. Each frame has a semaphore for concurrent processing. Allows multiple frames to be in-flight while still bounding the amount of work that piles up. One for each possible frame in flight.
	std::vector<VkFence>		framesInFlight;				//!< Similar to semaphores, but fences actually wait in our own code. Used to perform CPU-GPU synchronization. One for each possible frame in flight.
	std::vector<VkFence>		imagesInFlight;				//!< Maps frames in flight by their fences. Tracks for each swap chain image if a frame in flight is currently using it. One for each swap chain image.

	size_t						currentFrame;				//!< Frame to process next (0 or 1).
	bool						runThread;					//!< Signals whether the secondary thread (loadingThread) should be running.

	size_t						frameCount;					//!< Number of current frame being created [0, SIZE_MAX). If it's 0, no frame has been created yet. If render-loop finishes, the last value is kept. For debugging purposes.
	size_t						commandsCount;				//!< Number of drawing commands sent to the command buffer. For debugging purposes.

	// Main methods:

	/*
		@brief Allocates command buffers and record drawing commands in them. 
		
		Commands issued depends upon: SwapChainImages · Layer · Model · numRenders
		Bindings: pipeline > vertex buffer > indices > descriptor set > draw
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
	void renderLoop();

	/**
	*	Acquire image from swap chain, execute command buffer with that image as attachment in the framebuffer, and return the image to the swap chain for presentation.
	*	This method performs 3 operations asynchronously (the function call returns before the operations are finished, with undefined order of execution):
	*	<ul>
	*		<li>vkAcquireNextImageKHR: Acquire an image from the swap chain (imageAvailableSemaphores)</li>
	*		<li>vkQueueSubmit: Execute the command buffer with that image as attachment in the framebuffer (renderFinishedSemaphores, inFlightFences)</li>
	*		<li>vkQueuePresentKHR: Return the image to the swap chain for presentation</li>
	*	</ul>
	*	Each of the operations depend on the previous one finishing, so we need to synchronize the swap chain events.
	*	Two ways: semaphores (mainly designed to synchronize within or across command queues. Best fit here) and fences (mainly designed to synchronize your application itself with rendering operation).
	*	Synchronization examples: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present
	*/
	void drawFrame();

	/// Cleanup after render loop terminates
	void cleanup();

	// Update uniforms, transformation matrices, add/delete new models/textures, and submit command buffer. Transformation matrices (MVP) will be generated each frame.
	void updateStates(uint32_t currentImage);
	void(*userUpdate) (Renderer& rend, glm::mat4 view, glm::mat4 proj);

	/*
	@brief Check for pending items to load/delete (textures & models).
	
	<ul>Checking and loading process:
		<li> [mutSnapshot]: </li>
		<li>  texturesToLoad </li>
		<li>  modelsToLoad </li>
		<li>  modelsToDelete </li>
		<li>  texturesToDelete </li>
		<li> Texture::loadAndCreateTexture </li>
		<li> ModelData::fullConstruction </li>
		<li> modelsToDelete.erase </li>
		<li> texturesToDelete.erase </li>
	</ul>
*/
	void loadingThread();

	/// Used in drawFrame(). The window surface may change, making the swap chain no longer compatible with it (example: window resizing). Here, we catch these events and recreate the swap chain.
	void recreateSwapChain();

	/// Used in recreateSwapChain()
	void cleanupSwapChain();
	void stopThread();

public:

	// LOOK what if firstModel.size() == 0
	/// Constructor. Requires a callback for updating model matrix, adding models, deleting models, etc.
	Renderer(void(*graphicsUpdate)(Renderer&, glm::mat4 view, glm::mat4 proj), Camera* camera, size_t layers);
	~Renderer();

	/// Create command buffer and start render loop.
	int				run();

	/// Returns the timer object (provides access to time data).
	TimerSet&		getTimer();

	/// Returns the camera object (provides access to camera data).
	Camera&			getCamera();


	Input&			getInput();

	/**
		@brief Insert a partially initialized model object in modelsToLoad list.The loadModels_Thread() thread will fully initialize it as soon as possible.

		@param numberOfRenderings Number of times this model can be rendered in the same model. Recommendation: set this to the maximum number of rendering that will be.
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
	modelIterator	newModel(size_t layer, size_t numRenderings, primitiveTopology primitiveTopology, VertexLoader* vertexLoader, size_t numDynUBOs_vs, size_t dynUBOsize_vs, size_t dynUBOsize_fs, std::vector<texIterator>& textures, ShaderIter vertexShader, ShaderIter fragmentShader, bool transparency, uint32_t renderPassIndex = 0);
	void			deleteModel(modelIterator model);

	texIterator		newTexture(const char* path, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);												//!< Insert a partially initialized texture object in texturesToLoad list. Later, data from file will be loaded.
	texIterator		newTexture(unsigned char* pixels, unsigned texWidth, unsigned texHeight, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);	//!< Texture data from code
	void			deleteTexture(texIterator texture);

	ShaderIter		newShader(const std::string shaderFile, shaderc_shader_kind kind, bool optimize, bool isFile = true);
	void			deleteShader(ShaderIter shader);

	/**
	*	@brief 
	*/
	void			setRenders(modelIterator model, size_t numberOfRenders);

	/**
	*	@brief Not used
	*/
	size_t			getRendersCount(modelIterator model);

	/// Make a model the last to be drawn within its own layer. Useful for transparent objects.
	void			toLastDraw(modelIterator model);

	size_t getFrameCount();
	size_t getModelsCount();
	size_t getCommandsCount();
	float  getAspectRatio();
	glm::vec2 getScreenSize();

};

#endif