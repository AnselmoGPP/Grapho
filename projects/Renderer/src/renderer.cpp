// Definir principales objetos/elementes
// Abstraer creaci�n de modelos

//<<< Create a global VulkanEnvironment variable for sharing between ModelData objects <<<

#include <iostream>
#include <stdexcept>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <array>
#include <cstdint>				// UINT32_MAX
#include <algorithm>			// std::min / std::max
#include <fstream>
#include <chrono>
#include <unordered_map>		// For storing unique vertices from the model

#include "renderer.hpp"

Renderer::Renderer(void(*graphicsUpdate)(Renderer&))
	: input(e.window), graphicsUpdate(graphicsUpdate), currentFrame(0), runThread(false) { }

Renderer::~Renderer() 
{ 
	stopThread();
}

int Renderer::run()
{
	try 
	{
		createCommandBuffers();
		createSyncObjects();

		runThread = true;
		thread_loadModels = std::thread(&Renderer::loadModels_Thread, this);

		mainLoop();
		cleanup();
	}
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}

// (24)
void Renderer::createCommandBuffers()
{
	// Commmand buffer allocation
	commandBuffers.resize(e.swapChainFramebuffers.size());				// One commandBuffer per swapChainFramebuffer

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool			= e.commandPool;
	allocInfo.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;		// VK_COMMAND_BUFFER_LEVEL_ ... PRIMARY (can be submitted to a queue for execution, but cannot be called from other command buffers), SECONDARY (cannot be submitted directly, but can be called from primary command buffers - useful for reusing common operations from primary command buffers).
	allocInfo.commandBufferCount	= (uint32_t)commandBuffers.size();		// Number of buffers to allocate.

	if (vkAllocateCommandBuffers(e.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers!");
	
	// Start command buffer recording (one per swap chain framebuffer) and a render pass
	for (size_t i = 0; i < commandBuffers.size(); i++)
	{
		// Start command buffer recording
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags				= 0;			// [Optional] VK_COMMAND_BUFFER_USAGE_ ... ONE_TIME_SUBMIT_BIT (the command buffer will be rerecorded right after executing it once), RENDER_PASS_CONTINUE_BIT (secondary command buffer that will be entirely within a single render pass), SIMULTANEOUS_USE_BIT (the command buffer can be resubmitted while it is also already pending execution).
		beginInfo.pInheritanceInfo	= nullptr;		// [Optional] Only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers.

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)		// If a command buffer was already recorded once, this call resets it. It's not possible to append commands to a buffer at a later time.
			throw std::runtime_error("Failed to begin recording command buffer!");

		// Starting a render pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass			= e.renderPass;
		renderPassInfo.framebuffer			= e.swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset	= { 0, 0 };
		renderPassInfo.renderArea.extent	= e.swapChainExtent;						// Size of the render area (where shader loads and stores will take place). Pixels outside this region will have undefined values. It should match the size of the attachments for best performance.
		std::array<VkClearValue, 2> clearValues{};									// The order of clearValues should be identical to the order of your attachments.
		clearValues[0].color				= backgroundColor;										// Background color (alpha = 1 means 100% opacity)
		clearValues[1].depthStencil			= { 1.0f, 0 };									// Depth buffer range in Vulkan is [0.0, 1.0], where 1.0 lies at the far view plane and 0.0 at the near view plane. The initial value at each point in the depth buffer should be the furthest possible depth (1.0).
		renderPassInfo.clearValueCount		= static_cast<uint32_t>(clearValues.size());	// Clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we ...
		renderPassInfo.pClearValues			= clearValues.data();							// ... used as load operation for the color attachment and depth buffer.

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);		// VK_SUBPASS_CONTENTS_INLINE (the render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS (the render pass commands will be executed from secondary command buffers).

		// Basic drawing commands (for each model)
		for (modelIterator it = models.begin(); it != models.end(); it++)
		{
			if (it->numMM == 0) continue;

			//VkBuffer vertexBuffers[]	= { it->vertexBuffer };	// <<< Why not passing it directly (like the index buffer) instead of copying it? BTW, you are passing a local object to vkCmdBindVertexBuffers, how can it be possible?
			VkDeviceSize offsets[] = { 0 };	// <<<

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it->graphicsPipeline);// Second parameter: Specifies if the pipeline object is a graphics or compute pipeline.
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &it->vertexBuffer, offsets);				// Bind the vertex buffer to bindings.
			if(it->hasIndices())
				vkCmdBindIndexBuffer(commandBuffers[i], it->indexBuffer, 0, VK_INDEX_TYPE_UINT32);		// Bind the index buffer. VK_INDEX_TYPE_ ... UINT16, UINT32.

			for (size_t j = 0; j < it->numMM; j++)
			{
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it->pipelineLayout, 0, 1, &it->descriptorSets[i], 1, &it->dynamicOffsets[j]);
				if (it->hasIndices())
					vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(it->indices.size()), 1, 0, 0, 0);
				else
					vkCmdDraw(commandBuffers[i], it->vertices.size(), 1, 0, 0);

				//vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it->pipelineLayout, 0, 1, &it->descriptorSets[i], 0, nullptr);	// Bind the right descriptor set for each swap chain image to the descriptors in the shader.
				//vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(it->indices.size()), 1, 0, 0, 0);	// Draw the triangles using indices. Parameters: command buffer, number of indices, number of instances, offset into the index buffer, offset to add to the indices in the index buffer, offset for instancing. 												
				//vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);				// Draw the triangles without using indices. Parameters: command buffer, vertexCount (we have 3 vertices to draw), instanceCount (0 if you're doing instanced rendering), firstVertex (offset into the vertex buffer, lowest value of gl_VertexIndex), firstInstance (offset for instanced rendering, lowest value of gl_InstanceIndex).
			}

			// Instance rendering allows to perform a single draw call. As I understood, UBO can be passed in the following Ways:
			// - In a non-dynamic descriptor (problem: shader has to contain the declaration of one UBO for each instance).
			// - As a buffer's attribute (problem: it is non-modifyable).
			// - In a dynamic descriptor (solves both previous problems), but requires many draw calls (defeats the whole purpose of instance rendering).
			// https://stackoverflow.com/questions/54619507/whats-the-correct-way-to-implement-instanced-rendering-in-vulkan
			// https://www.reddit.com/r/vulkan/comments/hhoktq/rendering_multiple_objects/
		}

		// Finish up
		vkCmdEndRenderPass(commandBuffers[i]);
		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer!");
	}
}

// (25)
/// Create semaphores and fences for synchronizing the events occuring in each frame (drawFrame()).
void Renderer::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(e.swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(e.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(e.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(e.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization objects for a frame!");
		}
	}
}

void Renderer::mainLoop()
{
	timer.setMaxFPS(maxFPS);
	timer.startTimer();

	while (!glfwWindowShouldClose(e.window))
	{
		glfwPollEvents();	// Check for events (processes only those events that have already been received and then returns immediately)

		drawFrame();
		//if(modelsToLoad.size() > 0) addModelAndupdateCommandBuffers();

		if (glfwGetKey(e.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(e.window, true);
	}

	stopThread();

	vkDeviceWaitIdle(e.device);	// Waits for the logical device to finish operations. Needed for cleaning up once drawing and presentation operations (drawFrame) have finished. Use vkQueueWaitIdle for waiting for operations in a specific command queue to be finished.
}

void Renderer::stopThread()
{
	runThread = false;
	if (thread_loadModels.joinable()) thread_loadModels.join();
}

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
void Renderer::drawFrame()
{
	// Compute time difference
	timer.computeDeltaTime();

	// Wait for the frame to be finished. If VK_TRUE, we wait for all fences.
	vkWaitForFences(e.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	// Acquire an image from the swap chain
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(e.device, e.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);		// Swap chain is an extension feature. imageIndex: index to the VkImage in our swapChainImages.
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 					// VK_ERROR_OUT_OF_DATE_KHR: The swap chain became incompatible with the surface and can no longer be used for rendering. Usually happens after window resize.
		{ recreateSwapChain(); return; }
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)	// VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly.
		throw std::runtime_error("Failed to acquire swap chain image!");

	// Check if this image is being used. If used, wait. Then, mark it as used by this frame.
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)									// Check if a previous frame is using this image (i.e. there is its fence to wait on)
		vkWaitForFences(e.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];							// Mark the image as now being in use by this frame

	// Update uniforms and submit command buffer
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };		// Which semaphores to signal once the command buffers have finished execution.
	{
		const std::lock_guard<std::mutex> lock(mutex_modelsAndCommandBuffers);	// Controls access to model list and command buffer

		// Update uniforms
		updateUniformBuffer(imageIndex);

		// Submit the command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };			// Which semaphores to wait on before execution begins.
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };	// In which stages of the pipeline to wait the semaphore. VK_PIPELINE_STAGE_ ... TOP_OF_PIPE_BIT (ensures that the render passes don't begin until the image is available), COLOR_ATTACHMENT_OUTPUT_BIT (makes the render pass wait for this stage).
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		//submitInfo.pCommandBuffers = commandBuffers.data();			// Command buffers to submit for execution (here, the one that binds the swap chain image we just acquired as color attachment).

		vkResetFences(e.device, 1, &inFlightFences[currentFrame]);		// Reset the fence to the unsignaled state.

		{
			const std::lock_guard<std::mutex> lock(e.queueMutex);
			if (vkQueueSubmit(e.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)	// Submit the command buffer to the graphics queue. An array of VkSubmitInfo structs can be taken as argument when workload is much larger, for efficiency.
				throw std::runtime_error("Failed to submit draw command buffer!");
		}
	}

	// Note:
	// Subpass dependencies: Subpasses in a render pass automatically take care of image layout transitions. These transitions are controlled by subpass dependencies (specify memory and execution dependencies between subpasses).
	// There are two built-in dependencies that take care of the transition at the start and at the end of the render pass, but the former does not occur at the right time. It assumes that the transition occurs at the start of the pipeline, but we haven't acquired the image yet at that point. Two ways to deal with this problem:
	//		- waitStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT (ensures that the render passes don't begin until the image is available).
	//		- waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT (makes the render pass wait for this stage).

	// Presentation (submit the result back to the swap chain to have it eventually show up on the screen).
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount	= 1;
	presentInfo.pWaitSemaphores		= signalSemaphores;

	VkSwapchainKHR swapChains[]		= { e.swapChain };
	presentInfo.swapchainCount		= 1;
	presentInfo.pSwapchains			= swapChains;
	presentInfo.pImageIndices		= &imageIndex;
	presentInfo.pResults			= nullptr;			// Optional

	{
		const std::lock_guard<std::mutex> lock(e.queueMutex);
		result = vkQueuePresentKHR(e.presentQueue, &presentInfo);		// Submit request to present an image to the swap chain. Our triangle may look a bit different because the shader interpolates in linear color space and then converts to sRGB color space.
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || input.framebufferResized) {
		input.framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image!");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;	// By using the modulo operator (%), the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames.

	// vkQueueWaitIdle(presentQueue);							// Make the whole graphics pipeline to be used only one frame at a time (instead of using this, we use multiple semaphores for processing frames concurrently).
}

/// The window surface may change, making the swap chain no longer compatible with it (example: window resizing). Here, we catch these events and recreate the swap chain.
void Renderer::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(e.window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(e.window, &width, &height);
		glfwWaitEvents();
	}

	const std::lock_guard<std::mutex> lock(mutex_resizingWindow);

	vkDeviceWaitIdle(e.device);			// We shouldn't touch resources that may be in use.

	// Cleanup swapChain:
	cleanupSwapChain();

	// Recreate swapChain:
	//    - Environment
	e.recreateSwapChain();

	//    - Each model
	for (modelIterator it = models.begin(); it != models.end(); it++)
		it->recreateSwapChain();

	//    - Renderer
	createCommandBuffers();				// Command buffers directly depend on the swap chain images.
	imagesInFlight.resize(e.swapChainImages.size(), VK_NULL_HANDLE);
}

/// Update Uniform buffer. It will generate a new transformation every frame to make the geometry spin around.
void Renderer::updateUniformBuffer(uint32_t currentImage)
{
	// Update model matrices and other things (user defined)
	graphicsUpdate(*this);

	//static auto startTime	= std::chrono::high_resolution_clock::now();
	//static float prevTime	= 0;
	//auto currentTime		= std::chrono::high_resolution_clock::now();
	//float time			= std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	//float deltaTime		= time - prevTime;
	//prevTime				= time;
	
	// Compute transformation matrix
	input.cam.ProcessCameraInput(timer.getDeltaTime());
	glm::mat4 view = input.cam.GetViewMatrix();
	glm::mat4 proj = input.cam.GetProjectionMatrix(e.swapChainExtent.width / (float)e.swapChainExtent.height);
	//UniformBufferObject ubo{};
	//ubo.view = input.cam.GetViewMatrix();
	//ubo.proj = input.cam.GetProjectionMatrix(e.swapChainExtent.width / (float)e.swapChainExtent.height);

	// Copy the data in the uniform buffer object to the current uniform buffer
	// <<< Using a UBO this way is not the most efficient way to pass frequently changing values to the shader. Push constants are more efficient for passing a small buffer of data to shaders.
	for (modelIterator it = models.begin(); it != models.end(); it++)
	{
		if (it->numMM == 0) continue;

		UBOdynamic uboD(it->numMM, it->getUBORange());

		for (size_t i = 0; i < uboD.UBOcount; i++)
		{
			uboD.setModel(i, it->MM[i]);
			uboD.setView (i, view);
			uboD.setProj (i, proj);
		}

		void* data;
		vkMapMemory(e.device, it->uniformBuffersMemory[currentImage], 0, uboD.totalBytes, 0, &data);	// Get a pointer to some Vulkan/GPU memory of size X. vkMapMemory retrieves a host virtual address pointer (data) to a region of a mappable memory object (uniformBuffersMemory[]). We have to provide the logical device that owns the memory (e.device).
		memcpy(data, uboD.data, uboD.totalBytes);														// Copy some data in that memory. Copies a number of bytes (sizeof(ubo)) from a source (ubo) to a destination (data).
		vkUnmapMemory(e.device, it->uniformBuffersMemory[currentImage]);								// "Get rid" of the pointer. Unmap a previously mapped memory object (uniformBuffersMemory[]).
	}
}

/// Cleanup after render loop terminated
void Renderer::cleanup()
{
	// Cleanup renderer
	//cleanupSwapChain();

	// Renderer
	vkFreeCommandBuffers(e.device, e.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());	// Free Command buffers
	
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {							// Semaphores (render & image available) & fences (in flight)
		vkDestroySemaphore(e.device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(e.device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(e.device, inFlightFences[i], nullptr);
	}
	
	// Cleanup each model
	models.clear();
	modelsToLoad.clear();
	
	// Cleanup environment
	e.cleanupSwapChain();
	e.cleanup();
}

// Used in recreateSwapChain()
void Renderer::cleanupSwapChain()
{
	// Renderer (free Command buffers)
	vkFreeCommandBuffers(e.device, e.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	// Models
	for (modelIterator it = models.begin(); it != models.end(); it++)
		it->cleanupSwapChain();
		
	// Environment
	e.cleanupSwapChain();
}

// Inserts a partially initialized model. The thread_loadModels thread will fully initialize it as soon as possible. 
modelIterator Renderer::newModel(size_t numberOfRenderings, const char* modelPath, const char* texturePath, const char* VSpath, const char* FSpath, VertexType vertexType, primitiveTopology primitiveTopology, bool transparency)
{
	const std::lock_guard<std::mutex> lock(mutex_modelsToLoad);		// Control access to modelsToLoad list from newModel() and loadModels_Thread().

	return modelsToLoad.emplace(modelsToLoad.cend(), e, numberOfRenderings, modelPath, texturePath, VSpath, FSpath, vertexType, (VkPrimitiveTopology)primitiveTopology, transparency);

}

modelIterator Renderer::newModel(size_t numberOfRenderings, const VertexType& vertexType, size_t numVertex, const void* vertexData, std::vector<uint32_t>* indices, const char* texturePath, const char* VSpath, const char* FSpath, primitiveTopology primitiveTopology, bool transparency)
{
	const std::lock_guard<std::mutex> lock(mutex_modelsToLoad);		// Control access to modelsToLoad list from newModel() and loadModels_Thread().

	return modelsToLoad.emplace(modelsToLoad.cend(), e, numberOfRenderings, vertexType, numVertex, vertexData, indices, texturePath, VSpath, FSpath, (VkPrimitiveTopology)primitiveTopology, transparency);
}

void Renderer::deleteModel(modelIterator model)
{
	const std::lock_guard<std::mutex> lock(mutex_modelsToDelete);
	
	modelsToDelete.insert(modelsToDelete.cend(), model);
}

void Renderer::setRenders(modelIterator& model, size_t numberOfRenders)
{
	if (model->numMM != numberOfRenders)
	{
		const std::lock_guard<std::mutex> lock(mutex_rendersToSet);

		rendersToSet[&model] = numberOfRenders;

		if(model->MM.size() < numberOfRenders)		// Done to allow the user to update the new UBOs immediately
			model->MM.resize(numberOfRenders);
	}
}

// Check for models pending full initialization.
void Renderer::loadModels_Thread()
{
	modelIterator								lBegin, lEnd, lIt;	// Iterators for modelsToLoad (and deathRow)
	std::list<modelIterator>::iterator			dBegin, dEnd, dIt;	// Iterators for modelsToDelete
	std::map<modelIterator*, size_t>::iterator	rBegin, rEnd, rIt;	// Iterators for rendersToSet
	size_t models_to_load;
	size_t models_to_delete;
	size_t renders_to_set;
	std::list<ModelData> deathRow;			// Vulkan models that are going to be deleted
	bool commandBufferNotCreatedYet;		// True if run() has not been called yet (i.e. a command buffer has not been created yet)					
	
	while (runThread)
	{
		if (!rendersToSet.size() && !modelsToLoad.size() && !modelsToDelete.size())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
			continue;
		}
		else
		{
			models_to_delete = modelsToDelete.size();
			models_to_load   = modelsToLoad.size();
			renders_to_set   = rendersToSet.size();

			const std::lock_guard<std::mutex> lock(mutex_resizingWindow);

			// Fully initialize ModelData objects in the modelsToLoad list
			if (models_to_load)
			{
				lBegin = modelsToLoad.begin();
				lEnd   = modelsToLoad.end();

				for (lIt = lBegin; lIt != lEnd; ++lIt)
					lIt->fullConstruction();		// Load object data from memory and create Vulkan objects.
			}
			
			{
				const std::lock_guard<std::mutex> lock(mutex_modelsAndCommandBuffers);
				
				// Move objects from modelsToLoad list to models list
				if (models_to_load)
				{
					{
						const std::lock_guard<std::mutex> lock(mutex_modelsToLoad);

						models.splice(models.cend(), modelsToLoad, lBegin, lEnd);
					}
				}
								
				// Mark objects in models list for deleting them (they're pointed by the modelsToDelete list)
				if(models_to_delete)
				{
					{
						const std::lock_guard<std::mutex> lock(mutex_modelsToDelete);

						dBegin = modelsToDelete.begin();
						dEnd = modelsToDelete.end();

						for (dIt = dBegin; dIt != dEnd; dIt++)		// Move models to deathRow list
							deathRow.splice(deathRow.cend(), models, *dIt);

						modelsToDelete.erase(dBegin, dEnd);			// Delete iterators used
					}
				}
				
				// Delete existing command buffers
				commandBufferNotCreatedYet = (commandBuffers.size() == 0);
				if (!commandBufferNotCreatedYet)
				{
					vkDeviceWaitIdle(e.device);
					vkFreeCommandBuffers(e.device, e.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
				}

				// Set the number of renders for certain models in models list (this has to be here, since descriptors (and therefore, UBOs) are used by the command buffer
				if (renders_to_set)
				{
					{
						const std::lock_guard<std::mutex> lock(mutex_rendersToSet);

						rBegin = rendersToSet.begin();
						rEnd = rendersToSet.end();
						for (rIt = rBegin; rIt != rEnd; ++rIt)
							(*rIt->first)->resizeUBOset(rendersToSet[rIt->first]);

						rendersToSet.erase(rBegin, rEnd);
					}
				}
				
				// Generate new command buffer
				if (!commandBufferNotCreatedYet)
					createCommandBuffers();		// Create command buffers for all the models in the models list.
			}

			if (models_to_delete) 
				deathRow.clear();				// Delete models
		}
	}
}

TimerSet& Renderer::getTimer() { return timer; }

Camera& Renderer::getCamera() { return input.cam; }
