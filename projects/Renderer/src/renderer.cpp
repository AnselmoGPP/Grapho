
#include <iostream>
#include <stdexcept>
#include <cstdlib>				// EXIT_SUCCESS, EXIT_FAILURE
#include <array>
#include <cstdint>				// UINT32_MAX
#include <algorithm>			// std::min / std::max
#include <fstream>
#include <chrono>

#include "renderer.hpp"


Renderer::Renderer(void(*graphicsUpdate)(Renderer&, glm::mat4 view, glm::mat4 proj), Camera* camera, size_t layers)
	:
	input(e.c.window, camera), 
	numLayers(layers), 
	updateCommandBuffer(false), 
	userUpdate(graphicsUpdate), 
	currentFrame(0), 
	runThread(false),
	frameCount(0),
	commandsCount(0) 
{ 
	std::cout << "Hardware concurrency: " << (unsigned int)std::thread::hardware_concurrency << std::endl;
}

Renderer::~Renderer() { std::cout << __func__ << std::endl; }

int Renderer::run()
{
	std::cout << __func__ << "()" << std::endl;

	try 
	{
		createCommandBuffers();
		createSyncObjects();
		runThread = true;
		thread_loadModels = std::thread(&Renderer::loadingThread, this);

		renderLoop();
		cleanup();
	}
	catch (const std::exception& e) 
	{
		std::cerr << __func__ << "(): " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "run() end" << std::endl;
}

// (24)
void Renderer::createCommandBuffers()
{
	//std::cout << __func__ << "()" << std::endl;
	commandsCount = 0;

	// Commmand buffer allocation
	commandBuffers.resize(e.swapChain.images.size());
	
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool			= e.commandPool;
	allocInfo.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;		// VK_COMMAND_BUFFER_LEVEL_ ... PRIMARY (can be submitted to a queue for execution, but cannot be called from other command buffers), SECONDARY (cannot be submitted directly, but can be called from primary command buffers - useful for reusing common operations from primary command buffers).
	allocInfo.commandBufferCount	= (uint32_t)commandBuffers.size();		// Number of buffers to allocate.

	const std::lock_guard<std::mutex> lock(e.mutCommandPool);

	if (vkAllocateCommandBuffers(e.c.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers!");
	
	// Start command buffer recording (one per swapChainImage) and a render pass
	for (size_t i = 0; i < commandBuffers.size(); i++)
	{
		// Start command buffer recording
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;			// [Optional] VK_COMMAND_BUFFER_USAGE_ ... ONE_TIME_SUBMIT_BIT (the command buffer will be rerecorded right after executing it once), RENDER_PASS_CONTINUE_BIT (secondary command buffer that will be entirely within a single render pass), SIMULTANEOUS_USE_BIT (the command buffer can be resubmitted while it is also already pending execution).
		beginInfo.pInheritanceInfo = nullptr;		// [Optional] Only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers.

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)		// If a command buffer was already recorded once, this call resets it. It's not possible to append commands to a buffer at a later time.
			throw std::runtime_error("Failed to begin recording command buffer!");
		
		// Start render pass 1 (main color):
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = e.renderPass[0];
		renderPassInfo.framebuffer = e.framebuffers[i][0];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = e.swapChain.extent;					// Size of the render area (where shader loads and stores will take place). Pixels outside this region will have undefined values. It should match the size of the attachments for best performance.
		std::array<VkClearValue, 3> clearValues{};								// The order of clearValues should be identical to the order of your attachments.
		clearValues[0].color = backgroundColor;									// Resolve color buffer. Background color (alpha = 1 means 100% opacity)
		clearValues[1].depthStencil = { 1.0f, 0 };								// Depth buffer. Depth buffer range in Vulkan is [0.0, 1.0], where 1.0 lies at the far view plane and 0.0 at the near view plane. The initial value at each point in the depth buffer should be the furthest possible depth (1.0).
		clearValues[2].color = backgroundColor;									// MSAA color buffer.
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());// Clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we ...
		renderPassInfo.pClearValues = clearValues.data();						// ... used as load operation for the color attachment and depth buffer.
		
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);		// VK_SUBPASS_CONTENTS_INLINE (the render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS (the render pass commands will be executed from secondary command buffers).
		
		VkClearAttachment attachmentToClear;
		attachmentToClear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		attachmentToClear.clearValue.depthStencil = { 1.0f, 0 };

		VkClearRect rectangleToClear;
		rectangleToClear.rect.offset = { 0, 0 };
		rectangleToClear.rect.extent = e.swapChain.extent;
		rectangleToClear.baseArrayLayer = 0;
		rectangleToClear.layerCount = 1;

		VkDeviceSize offsets[] = { 0 };
		
		for (size_t j = 0; j < numLayers; j++)	// for each layer
		{
			vkCmdClearAttachments(commandBuffers[i], 1, &attachmentToClear, 1, &rectangleToClear);
			
			for (modelIterator it = models[0].begin(); it != models[0].end(); it++)	// for each model (color)
			{
				if (it->layer != j || !it->activeRenders) continue;

				vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it->graphicsPipeline);	// Second parameter: Specifies if the pipeline object is a graphics or compute pipeline.
				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &it->vertexBuffer, offsets);
				if (it->indices.size())
					vkCmdBindIndexBuffer(commandBuffers[i], it->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

				for (size_t k = 0; k < it->activeRenders; k++)	// for each rendering
				{
					commandsCount++;

					if (it->vsDynUBO.range)	// has UBO	<<< will this work ok if I don't have UBO for the vertex shader but a UBO for the fragment shader?
						vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it->pipelineLayout, 0, 1, &it->descriptorSets[i], 1, &it->vsDynUBO.dynamicOffsets[k]);
					else
						vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it->pipelineLayout, 0, 1, &it->descriptorSets[i], 0, 0);

					if (it->indices.size())	// has indices
						vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(it->indices.size()), 1, 0, 0, 0);
					else
						vkCmdDraw(commandBuffers[i], it->vertices.size(), 1, 0, 0);
				}
			}
		}

		vkCmdEndRenderPass(commandBuffers[i]);

		// Start render pass 2 (post processing):

		renderPassInfo.renderPass = e.renderPass[1];
		renderPassInfo.framebuffer = e.framebuffers[i][1];
		clearValues[0].color = backgroundColor;
		clearValues[1].depthStencil = { 1.0f, 0 };
		clearValues[2].color = backgroundColor;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);	// Start render pass
		//vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);						// Start subpass
		
		for (modelIterator it = models[1].begin(); it != models[1].end(); it++)	// for each model (post processing)
		{
			if (!it->activeRenders) continue;
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it->graphicsPipeline);	// Second parameter: Specifies if the pipeline object is a graphics or compute pipeline.
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &it->vertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], it->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, it->pipelineLayout, 0, 1, &it->descriptorSets[i], 1, &it->vsDynUBO.dynamicOffsets[0]);
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(it->indices.size()), 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(commandBuffers[i]);
		
		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer!");
	}

	updateCommandBuffer = false;
}

// (25)
void Renderer::createSyncObjects()
{
	std::cout << __func__ << "()" << std::endl;

	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	framesInFlight.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(e.swapChain.images.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(e.c.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(e.c.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(e.c.device, &fenceInfo, nullptr, &framesInFlight[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization objects for a frame!");
		}
	}
}

void Renderer::renderLoop()
{
	std::cout << __func__ << "()" << std::endl;

	frameCount = 0;
	timer.setMaxFPS(maxFPS);
	timer.startTimer();

	while (!glfwWindowShouldClose(e.c.window))
	{
		++frameCount;

		glfwPollEvents();	// Check for events (processes only those events that have already been received and then returns immediately)
		drawFrame();

		if (glfwGetKey(e.c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(e.c.window, true);
	}

	stopThread();

	vkDeviceWaitIdle(e.c.device);	// Waits for the logical device to finish operations. Needed for cleaning up once drawing and presentation operations (drawFrame) have finished. Use vkQueueWaitIdle for waiting for operations in a specific command queue to be finished.
}

void Renderer::stopThread()
{
	runThread = false;
	if (thread_loadModels.joinable()) thread_loadModels.join();
}

/*
	-Wait for the frame's CB execution (inFlightFences)
	vkAcquireNextImageKHR (acquire swap chain image)
	-Wait for the swap chain image's CB execution (imagesInFlight/inFlightFences)
	Update CB (optional)
	-Wait for acquiring a swap chain image (imageAvailableSemaphores)
	vkQueueSubmit (execute CB)
	-Wait for CB execution (renderFinishedSemaphores)
	vkQueuePresentKHR (present image)
	
	waitFor(framesInFlight[currentFrame]);
	vkAcquireNextImageKHR(imageAvailableSemaphores[currentFrame], imageIndex);
	waitFor(imagesInFlight[imageIndex]);
	imagesInFlight[imageIndex] = framesInFlight[currentFrame];
	updateCB();
	vkResetFences(framesInFlight[currentFrame])
	vkQueueSubmit(renderFinishedSemaphores[currentFrame], framesInFlight[currentFrame]); // waitFor(imageAvailableSemaphores[currentFrame])
	vkQueuePresentKHR(); // waitFor(renderFinishedSemaphores[currentFrame]);
	currentFrame = nextFrame;
*/
void Renderer::drawFrame()
{
	// Wait for the frame to be finished (command buffer execution). If VK_TRUE, we wait for all fences.
	vkWaitForFences(e.c.device, 1, &framesInFlight[currentFrame], VK_TRUE, UINT64_MAX);

	// Acquire an image from the swap chain
	uint32_t imageIndex;		// Swap chain image index (0, 1, 2)
	VkResult result = vkAcquireNextImageKHR(e.c.device, e.swapChain.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);		// Swap chain is an extension feature. imageIndex: index to the VkImage in our swapChainImages.
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 					// VK_ERROR_OUT_OF_DATE_KHR: The swap chain became incompatible with the surface and can no longer be used for rendering. Usually happens after window resize.
	{ 
		std::cout << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
		recreateSwapChain(); 
		return; 
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)	// VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly.
		throw std::runtime_error("Failed to acquire swap chain image!");

	// Check if this image is being used. If used, wait. Then, mark it as used by this frame.
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)									// Check if a previous frame is using this image (i.e. there is its fence to wait on)
		vkWaitForFences(e.c.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	imagesInFlight[imageIndex] = framesInFlight[currentFrame];							// Mark the image as now being in use by this frame

	updateStates(imageIndex);

	// Submit the command buffer
	VkSubmitInfo submitInfo{};
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };			// Which semaphores to wait on before execution begins.
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };		// Which semaphores to signal once the command buffers have finished execution.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };	// In which stages of the pipeline to wait the semaphore. VK_PIPELINE_STAGE_ ... TOP_OF_PIPE_BIT (ensures that the render passes don't begin until the image is available), COLOR_ATTACHMENT_OUTPUT_BIT (makes the render pass wait for this stage).
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;	// Semaphores upon which to wait before the CB/s begin execution.
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;// Semaphores to be signaled once the CB/s have completed execution.
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];		// Command buffers to submit for execution (here, the one that binds the swap chain image we just acquired as color attachment).

	vkResetFences(e.c.device, 1, &framesInFlight[currentFrame]);		// Reset the fence to the unsignaled state.

	{
		const std::lock_guard<std::mutex> lock(e.queueMutex);
		if (vkQueueSubmit(e.c.graphicsQueue, 1, &submitInfo, framesInFlight[currentFrame]) != VK_SUCCESS)	// Submit the command buffer to the graphics queue. An array of VkSubmitInfo structs can be taken as argument when workload is much larger, for efficiency.
			throw std::runtime_error("Failed to submit draw command buffer!");
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

	VkSwapchainKHR swapChains[]		= { e.swapChain.swapChain };
	presentInfo.swapchainCount		= 1;
	presentInfo.pSwapchains			= swapChains;
	presentInfo.pImageIndices		= &imageIndex;
	presentInfo.pResults			= nullptr;			// Optional

	{
		const std::lock_guard<std::mutex> lock(e.queueMutex);
		result = vkQueuePresentKHR(e.c.presentQueue, &presentInfo);		// Submit request to present an image to the swap chain. Our triangle may look a bit different because the shader interpolates in linear color space and then converts to sRGB color space.
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || input.framebufferResized) 
	{
		std::cout << "Out-of-date/Suboptimal KHR or window resized" << std::endl;
		input.framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image!");
	
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;	// By using the modulo operator (%), the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames.

	//vkQueueWaitIdle(e.presentQueue);							// Make the whole graphics pipeline to be used only one frame at a time (instead of using this, we use multiple semaphores for processing frames concurrently).
}

void Renderer::recreateSwapChain()
{
	std::cout << __func__ << "()" << std::endl;

	// Get window size
	int width = 0, height = 0;
	glfwGetFramebufferSize(e.c.window, &width, &height);
	while (width == 0 || height == 0) 
	{
		glfwGetFramebufferSize(e.c.window, &width, &height);
		glfwWaitEvents();
	}
	e.c.width = width;
	e.c.height = height;

	vkDeviceWaitIdle(e.c.device);			// We shouldn't touch resources that may be in use.

	// Cleanup swapChain:
	cleanupSwapChain();

	// Recreate swapChain:
	//    - Environment
	e.recreate_Images_RenderPass_SwapChain();

	//    - Each model
	for (uint32_t i = 0; i < e.c.numRenderPasses; i++)
		for (modelIterator it = models[i].begin(); it != models[i].end(); it++)
			it->recreate_Pipeline_Descriptors();

	//    - Renderer
	createCommandBuffers();				// Command buffers directly depend on the swap chain images.
	imagesInFlight.resize(e.swapChain.images.size(), VK_NULL_HANDLE);
}

void Renderer::cleanupSwapChain()
{
	std::cout << __func__ << "()" << std::endl;

	{
		const std::lock_guard<std::mutex> lock(e.mutCommandPool);
		vkQueueWaitIdle(e.c.graphicsQueue);
		vkFreeCommandBuffers(e.c.device, e.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	}

	// Models
	for(uint32_t i = 0; i < e.c.numRenderPasses; i++)
		for (modelIterator it = models[i].begin(); it != models[i].end(); it++)
			it->cleanup_Pipeline_Descriptors();

	// Environment
	e.cleanup_Images_RenderPass_SwapChain();
}

void Renderer::cleanup()
{
	std::cout << __func__ << "() 1" << std::endl;

	// Cleanup renderer
	//cleanupSwapChain();

	// Renderer
	{
		const std::lock_guard<std::mutex> lock(e.mutCommandPool);
		vkQueueWaitIdle(e.c.graphicsQueue);
		vkFreeCommandBuffers(e.c.device, e.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());	// Free Command buffers
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {							// Semaphores (render & image available) & fences (in flight)
		vkDestroySemaphore(e.c.device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(e.c.device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(e.c.device, framesInFlight[i], nullptr);
	}

	std::cout << __func__ << "() 2" << std::endl;

	// Cleanup each model
	models[0].clear();
	models[1].clear();
	modelsToLoad.clear();
	
	// Cleanup textures
	textures.clear();

	// Cleanup shaders
	for (ShaderIter it = shaders.begin(); it != shaders.end(); it++)
		vkDestroyShaderModule(e.c.device, *it, nullptr);
	shaders.clear();

	// Cleanup environment
	std::cout << "   >>> Buffers size: " << models[0].size() << "(m1), " << models[1].size() << "(m2), " << modelsToLoad.size() << "(ml), " << modelsToDelete.size() << "(md), " << texturesToLoad.size() << "(tl), " << texturesToDelete.size() << "(td)" << std::endl;
	e.cleanup_Images_RenderPass_SwapChain();
	e.cleanup(); 

	std::cout << "Cleanup() end" << std::endl;
}

modelIterator Renderer::newModel(size_t layer, size_t numRenderings, primitiveTopology primitiveTopology, VertexLoader* vertexLoader, size_t numDynUBOs_vs, size_t dynUBOsize_vs, size_t dynUBOsize_fs, std::vector<texIterator>& textures, ShaderIter vertexShader, ShaderIter fragmentShader, bool transparency, uint32_t renderPassIndex)
{
	return modelsToLoad.emplace(
		modelsToLoad.cend(), 
		e, 
		layer,
		numRenderings, 
		(VkPrimitiveTopology) primitiveTopology, 
		vertexLoader,
		numDynUBOs_vs, dynUBOsize_vs, dynUBOsize_fs,
		textures, 
		vertexShader, fragmentShader,
		transparency,
		renderPassIndex);
}

void Renderer::deleteModel(modelIterator model)	// <<< splice an element only knowing the iterator (no need to check lists)?
{
	for(uint32_t i = 0; i < e.c.numRenderPasses; i++)
		for (modelIterator it = models[i].begin(); it != models[i].end(); it++)
			if (it == model)
			{
				model->inModels = false;
				modelsToDelete.splice(modelsToDelete.cend(), models[i], model);
				updateCommandBuffer = true;
				return;
			}

	for (modelIterator it = modelsToLoad.begin(); it != modelsToLoad.end(); it++)
		if (it == model)
		{
			model->inModels = false;
			modelsToDelete.splice(modelsToDelete.cend(), modelsToLoad, model);
			return;
		}
}

texIterator Renderer::newTexture(const char* path, VkFormat imageFormat, VkSamplerAddressMode addressMode)
{
	return texturesToLoad.emplace(texturesToLoad.cend(), path, imageFormat, addressMode);
}

texIterator Renderer::newTexture(unsigned char* pixels, unsigned texWidth, unsigned texHeight, VkFormat imageFormat, VkSamplerAddressMode addressMode)
{
	return texturesToLoad.emplace(texturesToLoad.cend(), pixels, texWidth, texHeight, imageFormat, addressMode);
}

void Renderer::deleteTexture(texIterator texture)	// <<< splice an element only knowing the iterator (no need to check lists)?
{
	bool notLoadedYet = false;
	for (texIterator it = texturesToLoad.begin(); it != texturesToLoad.end(); it++)
		if (it == texture) { notLoadedYet = true; break; }

	if (notLoadedYet)
		texturesToDelete.splice(texturesToDelete.cend(), texturesToLoad, texture);
	else
		texturesToDelete.splice(texturesToDelete.cend(), textures, texture);
}

ShaderIter Renderer::newShader(const std::string shaderFile, shaderc_shader_kind shaderKind, bool optimize, bool isFile)
{
	// Get data from txt file:

	size_t fileSize = 0;
	std::vector<char> glslData;
	std::string fileName;

	if (isFile)		// shaderFile == shader file name
	{
		std::cout << __func__ << "(): " << shaderFile << std::endl;

		fileName = shaderFile;

		std::ifstream file(shaderFile, std::ios::ate | std::ios::binary);		// ate: Start reading at the end of the file  /  binary: Read file as binary file (avoid text transformations)
		if (!file.is_open())
			throw std::runtime_error("Failed to open file!");

		fileSize = (size_t)file.tellg();
		glslData.resize(fileSize);

		file.seekg(0);
		file.read(glslData.data(), fileSize);

		file.close();
	}
	else			// shaderFile == shader content
	{
		std::cout << __func__ << "(): Hardcoded shader" << std::endl;

		fileName = "HardcodedShader";
		fileSize = shaderFile.size();
		glslData.resize(fileSize);
		std::copy(shaderFile.begin(), shaderFile.end(), glslData.begin());
	}
	
	// Compile data:
	
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_performance);

	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(glslData.data(), glslData.size(), shaderKind, fileName.c_str(), options);
	if (module.GetCompilationStatus() != shaderc_compilation_status_success)
		std::cerr << "Error compiling shader module - " << module.GetErrorMessage() << std::endl;
	
	std::vector<uint32_t> spirv = { module.cbegin(), module.cend() };

	//Create shader module:

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = reinterpret_cast<const uint32_t*>(spirv.data());	// The default allocator from std::vector ensures that the data satisfies the alignment requirements of `uint32_t`.

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(e.c.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module!");

	shaders.push_back(shaderModule);
	return (--shaders.end());
}

void Renderer::deleteShader(ShaderIter shader)
{
	vkDestroyShaderModule(e.c.device, *shader, nullptr);
	shaders.erase(shader);
}

void Renderer::setRenders(modelIterator model, size_t numberOfRenders)
{
	if (numberOfRenders != model->activeRenders)
	{
		model->setRenderCount(numberOfRenders);

		updateCommandBuffer = true;		//We are flagging commandBuffer for update assuming that our model is in list "model"
	}
}

void Renderer::loadingThread()
{
	std::cout << __func__ << "()" << std::endl;

	texIterator beginTexLoad;
	modelIterator beginModLoad;
	modelIterator endModDelete;
	texIterator endTexDelete;
	size_t countTexLoad;
	size_t countModLoad;
	size_t countModDelete;
	size_t countTexDelete;

	while (runThread)
	{
		// Snapshot of lists state: Get iterator to first non-constructed element, and the number of elements to construct.
		{
			const std::lock_guard<std::mutex> lock(mutSnapshot);

			if (texturesToLoad.size() && !(--texturesToLoad.end())->fullyConstructed)
			{
				countTexLoad = texturesToLoad.size();
				for (beginTexLoad = texturesToLoad.begin(); beginTexLoad != texturesToLoad.end(); beginTexLoad++)
				{
					if (beginTexLoad->fullyConstructed) --countTexLoad;
					else break;
				}
			}
			else countTexLoad = 0;

			if (modelsToLoad.size() && !(--modelsToLoad.end())->fullyConstructed)
			{
				countModLoad = modelsToLoad.size();
				for (beginModLoad = modelsToLoad.begin(); beginModLoad != modelsToLoad.end(); beginModLoad++)
				{
					if (beginModLoad->fullyConstructed) --countModLoad;
					else break;
				}
			}
			else countModLoad = 0;

			countModDelete = modelsToDelete.size();

			countTexDelete = texturesToDelete.size();
		}

		// Construct or destroy elements
		if (countTexLoad || countModLoad || countModDelete || countTexDelete)
		{
			// Textures to load
			if(countTexLoad)
			{
				while (countTexLoad)
				{
					beginTexLoad->loadAndCreateTexture(&e);
					++beginTexLoad;
					--countTexLoad;
				}
			}

			// Models to load
			if (countModLoad)
			{
				while (countModLoad)
				{
					beginModLoad->fullConstruction();
					++beginModLoad;						// <<< Problem? updateCB Vs loadingThread
					--countModLoad;
				}
			}

			// Models to delete
			if (countModDelete)
			{
				endModDelete = modelsToDelete.begin();
				std::advance(endModDelete, countModDelete);
				modelsToDelete.erase(modelsToDelete.begin(), endModDelete);
				//modelsToDelete.clear();
			}

			// Textures to delete
			if (countTexDelete)
			{
				endTexDelete = texturesToDelete.begin();
				std::advance(endTexDelete, countTexDelete);
				texturesToDelete.erase(texturesToDelete.begin(), endTexDelete);
				//texturesToDelete.clear();
			}

		}
		else 
			std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
	}
	
	std::cout << __func__ << "() end" << std::endl;
}

void Renderer::updateStates(uint32_t currentImage)
{
	const std::lock_guard<std::mutex> lock(mutSnapshot);

	// - USER UPDATES

	timer.computeDeltaTime();

	// Compute transformation matrix
	input.cam->ProcessCameraInput(input.window, timer.getDeltaTime());
	glm::mat4 view = input.cam->GetViewMatrix();
	glm::mat4 proj = input.cam->GetProjectionMatrix(e.swapChain.extent.width / (float)e.swapChain.extent.height);

	//UniformBufferObject ubo{};
	//ubo.view = input.cam.GetViewMatrix();
	//ubo.proj = input.cam.GetProjectionMatrix(e.swapChainExtent.width / (float)e.swapChainExtent.height);

	// Update model matrices and other things (user defined)
	userUpdate(*this, view, proj);

	// - MOVE MODELS AND TEXTURES

	uint32_t i;

	if (texturesToLoad.size() && texturesToLoad.begin()->fullyConstructed)
	{
		texIterator begin, end;
		begin = end = texturesToLoad.begin();
		while (end->fullyConstructed && end != texturesToLoad.end()) ++end;
		textures.splice(textures.cend(), texturesToLoad, begin, end);
	}

	if (modelsToLoad.size() && modelsToLoad.begin()->fullyConstructed)
	{
		modelIterator begin, it, prev;
		begin = it = modelsToLoad.begin();
		while (it->fullyConstructed && it != modelsToLoad.end()) 
		{
			it->inModels = true;
			prev = it;
			++it;

			i = prev->renderPassIndex;
			models[i].splice(models[i].cend(), modelsToLoad, prev);
			updateCommandBuffer = true;
		}
	}

	while (lastModelsToDraw.size())
	{
		if (lastModelsToDraw[0]->inModels)
		{
			i = lastModelsToDraw[0]->renderPassIndex;
			models[i].splice(models[i].end(), models[i], lastModelsToDraw[0]);
		}
		lastModelsToDraw.erase(lastModelsToDraw.begin());
	}
	
	// - COPY DATA FROM UBOS TO GPU MEMORY

	// Copy the data in the uniform buffer object to the current uniform buffer
	// <<< Using a UBO this way is not the most efficient way to pass frequently changing values to the shader. Push constants are more efficient for passing a small buffer of data to shaders.
	for(i = 0; i < e.c.numRenderPasses; i++)
		for (modelIterator it = models[i].begin(); it != models[i].end(); it++)
		{
			if (it->vsDynUBO.totalBytes)
			{
				void* data;
				vkMapMemory(e.c.device, it->vsDynUBO.uniformBuffersMemory[currentImage], 0, it->vsDynUBO.totalBytes, 0, &data);	// Get a pointer to some Vulkan/GPU memory of size X. vkMapMemory retrieves a host virtual address pointer (data) to a region of a mappable memory object (uniformBuffersMemory[]). We have to provide the logical device that owns the memory (e.device).
				memcpy(data, it->vsDynUBO.ubo.data(), it->vsDynUBO.totalBytes);														// Copy some data in that memory. Copies a number of bytes (sizeof(ubo)) from a source (ubo) to a destination (data).
				vkUnmapMemory(e.c.device, it->vsDynUBO.uniformBuffersMemory[currentImage]);								// "Get rid" of the pointer. Unmap a previously mapped memory object (uniformBuffersMemory[]).
			}

			if (it->fsUBO.totalBytes)
			{
				void* data;
				vkMapMemory(e.c.device, it->fsUBO.uniformBuffersMemory[currentImage], 0, it->fsUBO.totalBytes, 0, &data);
				memcpy(data, it->fsUBO.ubo.data(), it->fsUBO.totalBytes);
				vkUnmapMemory(e.c.device, it->fsUBO.uniformBuffersMemory[currentImage]);
			}
		}

	// - UPDATE COMMAND BUFFER
	if (updateCommandBuffer)
	{
		{
			const std::lock_guard<std::mutex> lock(e.mutCommandPool);
			vkQueueWaitIdle(e.c.graphicsQueue);
			vkFreeCommandBuffers(e.c.device, e.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());	// Any primary command buffer that is in the recording or executable state and has any element of pCommandBuffers recorded into it, becomes invalid.
		}

		createCommandBuffers();
	}
}

TimerSet& Renderer::getTimer() { return timer; }

Camera& Renderer::getCamera() { return *input.cam; }

Input& Renderer::getInput() { return input; }

size_t Renderer::getRendersCount(modelIterator model) { return model->activeRenders; }

void Renderer::toLastDraw(modelIterator model)
{
	lastModelsToDraw.push_back(model);
}

size_t Renderer::getFrameCount() { return frameCount; }

size_t Renderer::getModelsCount() { return models[0].size() + models[1].size(); }

size_t Renderer::getCommandsCount() { return commandsCount; }

float Renderer::getAspectRatio() { return (float)e.c.height / e.c.width; }

glm::vec2 Renderer::getScreenSize() { return glm::vec2(e.c.width, e.c.height); }

