
#include "models.hpp"
#include "commons.hpp"


std::vector<texIterator> noTextures;
std::vector<uint16_t> noIndices;

ModelData::ModelData(VulkanEnvironment& environment, size_t layer, size_t activeRenders, VkPrimitiveTopology primitiveTopology, VertexLoader* vertexLoader, size_t numDynUBOs_vs, size_t dynUBOsize_vs, size_t dynUBOsize_fs, std::vector<texIterator>& textures, ShaderIter vertexShader, ShaderIter fragmentShader, bool transparency)
	: e(environment),
	primitiveTopology(primitiveTopology),
	vertexShader(vertexShader),
	fragmentShader(fragmentShader),
	hasTransparencies(transparency),
	textures(textures),
	vertices(vertexLoader->getVertexType()),				// Done for calling the correct getAttributeDescriptions() and getBindingDescription() in createGraphicsPipeline()
	vsDynUBO(e, numDynUBOs_vs, dynUBOsize_vs, e.minUniformBufferOffsetAlignment),
	fsUBO(e, dynUBOsize_fs ? 1 : 0, dynUBOsize_fs, e.minUniformBufferOffsetAlignment),
	layer(layer),
	activeRenders(activeRenders),
	fullyConstructed(false),
	inModels(false)
{
	this->vertexLoader = vertexLoader;
	this->vertexLoader->setDestination(vertices, indices);

	//if (fsUBO.range) fsUBO.resize(1);
	//resizeUBOset(numRenderings);
}

ModelData::~ModelData()
{
	//std::cout << __func__ << "() (" << FSpath << ')' << std::endl;

	if (fullyConstructed) {
		cleanupSwapChain();
		cleanup();
	}

	delete vertexLoader;
}

ModelData& ModelData::fullConstruction()
{
	createDescriptorSetLayout();
	createGraphicsPipeline();

	vertexLoader->loadVertex();
	createVertexBuffer();
	if(indices.size()) createIndexBuffer();

	vsDynUBO.createUniformBuffers();
	fsUBO.createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();

	fullyConstructed = true;
	return *this;
}

// (9)
void ModelData::createDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	uint32_t bindNumber = 0;

	//	Dynamic Uniform buffer descriptor (vertex shader)
	if (vsDynUBO.range)
	{
		VkDescriptorSetLayoutBinding vsUboLayoutBinding{};
		vsUboLayoutBinding.binding = bindNumber++;
		vsUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;	// VK_DESCRIPTOR_TYPE_ ... UNIFORM_BUFFER, UNIFORM_BUFFER_DYNAMIC
		vsUboLayoutBinding.descriptorCount = 1;											// In case you want to specify an array of UBOs <<< (example: for specifying a transformation for each of bone in a skeleton for skeletal animation).
		vsUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;						// Tell in which shader stages the descriptor will be referenced. This field can be a combination of VkShaderStageFlagBits values or the value VK_SHADER_STAGE_ALL_GRAPHICS.
		vsUboLayoutBinding.pImmutableSamplers = nullptr;								// [Optional] Only relevant for image sampling related descriptors.

		bindings.push_back(vsUboLayoutBinding);
	}

	// Uniform buffer descriptor (fragment shader)
	if (fsUBO.range)
	{
		VkDescriptorSetLayoutBinding fsUboLayoutBinding{};
		fsUboLayoutBinding.binding = bindNumber++;
		fsUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		fsUboLayoutBinding.descriptorCount = 1;
		fsUboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		fsUboLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(fsUboLayoutBinding);
	}

	//	Combined image sampler descriptor (it lets shaders access an image resource through a sampler object)
	if (textures.size())
	{
		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = bindNumber++;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = textures.size();
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;			// We want to use the combined image sampler descriptor in the fragment shader. It's possible to use texture sampling in the vertex shader (example: to dynamically deform a grid of vertices by a heightmap).
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(samplerLayoutBinding);
	}

	// Create a descriptor set layout (combines all of the descriptor bindings)
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(e.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");
}

// (10)
void ModelData::createGraphicsPipeline()
{
	// Create pipeline layout   <<< sameMod
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;						// Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;		// Optional	 <<<<<
	pipelineLayoutInfo.pushConstantRangeCount = 0;				// Optional. <<< Push constants are another way of passing dynamic values to shaders.
	pipelineLayoutInfo.pPushConstantRanges = nullptr;			// Optional

	if (vkCreatePipelineLayout(e.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout!");

	// Read shader files
	//std::vector<char> vertShaderCode = readFile(VSpath);
	//std::vector<char> fragShaderCode = readFile(FSpath);
	//VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	//VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
	
	// Configure Vertex shader
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = *vertexShader;
	vertShaderStageInfo.pName = "main";						// Function to invoke (entrypoint). You may combine multiple fragment shaders into a single shader module and use different entry points (different behaviors).  
	vertShaderStageInfo.pSpecializationInfo = nullptr;		// Optional. Specifies values for shader constants.

	// Configure Fragment shader
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = *fragmentShader;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// Vertex input: Describes format of the vertex data that will be passed to the vertex shader.
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = vertices.Vtype.getBindingDescription();
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;							// Optional
	auto attributeDescriptions = vertices.Vtype.getAttributeDescriptions();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();				// Optional

	// Input assembly: Describes what kind of geometry will be drawn from the vertices, and if primitive restart should be enabled.
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = primitiveTopology;		// VK_PRIMITIVE_TOPOLOGY_ ... POINT_LIST, LINE_LIST, LINE_STRIP, TRIANGLE_LIST, TRIANGLE_STRIP
	inputAssembly.primitiveRestartEnable = VK_FALSE;					// If VK_TRUE, then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.

	// Viewport: Describes the region of the framebuffer that the output will be rendered to.
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)e.swapChainExtent.width;
	viewport.height = (float)e.swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Scissor rectangle: Defines in which region pixels will actually be stored. Pixels outside the scissor rectangles will be discarded by the rasterizer. It works like a filter rather than a transformation.
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = e.swapChainExtent;

	// Viewport state: Combines the viewport and scissor rectangle into a viewport state. Multiple viewports and scissors require enabling a GPU feature.
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer: It takes the geometry shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader. It also performs depth testing, face culling and the scissor test, and can be configured to output fragments that fill entire polygons or just the edges (wireframe rendering).
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;							// If VK_TRUE, fragments that are beyond the near and far planes are clamped to them (requires enabling a GPU feature), as opposed to discarding them.
	rasterizer.rasterizerDiscardEnable = VK_FALSE;							// If VK_TRUE, geometry never passes through the rasterizer stage (disables any output to the framebuffer).
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;				// How fragments are generated for geometry (VK_POLYGON_MODE_ ... FILL, LINE, POINT). Any mode other than FILL requires enabling a GPU feature.
	rasterizer.lineWidth = LINE_WIDTH;								// Thickness of lines in terms of number of fragments. The maximum line width supported depends on the hardware. Lines thicker than 1.0f requires enabling the `wideLines` GPU feature.
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;			// Type of face culling (disable culling, cull front faces, cull back faces, cull both).
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Vertex order for faces to be considered front-facing (clockwise, counterclockwise). If we draw vertices clockwise, because of the Y-flip we did in the projection matrix, the vertices are now drawn counter-clockwise.
	rasterizer.depthBiasEnable = VK_FALSE;							// If VK_TRUE, it allows to alter the depth values (sometimes used for shadow mapping).
	rasterizer.depthBiasConstantFactor = 0.0f;								// [Optional] 
	rasterizer.depthBiasClamp = 0.0f;								// [Optional] 
	rasterizer.depthBiasSlopeFactor = 0.0f;								// [Optional] 

	// Multisampling: One way to perform anti-aliasing. Combines the fragment shader results of multiple polygons that rasterize to the same pixel. Requires enabling a GPU feature.
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = e.msaaSamples;
	multisampling.sampleShadingEnable = (e.add_SS ? VK_TRUE : VK_FALSE);	// Enable sample shading in the pipeline
	if (e.add_SS)
		multisampling.minSampleShading = .2f;								// [Optional] Min fraction for sample shading; closer to one is smoother
	multisampling.pSampleMask = nullptr;									// [Optional]
	multisampling.alphaToCoverageEnable = VK_FALSE;							// [Optional]
	multisampling.alphaToOneEnable = VK_FALSE;								// [Optional]

	// Depth and stencil testing. Used if you are using a depth and/or stencil buffer.
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;				// Specify if the depth of new fragments should be compared to the depth buffer to see if they should be discarded.
	depthStencil.depthWriteEnable = VK_TRUE;				// Specify if the new depth of fragments that pass the depth test should actually be written to the depth buffer.
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;	// Specify the comparison that is performed to keep or discard fragments.
	depthStencil.depthBoundsTestEnable = VK_FALSE;				// [Optional] Use depth bound test (allows to only keep fragments that fall within a specified depth range.
	depthStencil.minDepthBounds = 0.0f;					// [Optional]
	depthStencil.maxDepthBounds = 1.0f;					// [Optional]
	depthStencil.stencilTestEnable = VK_FALSE;				// [Optional] Use stencil buffer operations (if you want to use it, make sure that the format of the depth/stencil image contains a stencil component).
	depthStencil.front = {};					// [Optional]
	depthStencil.back = {};					// [Optional]

	// Color blending: After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. Two ways to do it: Mix old and new value to produce a final color, or combine the old and new value using a bitwise operation.
	//	- Configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	if (!hasTransparencies)	// Not alpha blending implemented
	{
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional. Check VkBlendFactor enum.
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	// Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;				// Optional. Check VkBlendOp enum.
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	// Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;				// Optional
	}
	else	// Options for implementing alpha blending (new color blended with old color based on its opacity):
	{
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		/* Pseudocode demonstration:
			if (blendEnable) {
				finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
				finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
			}
			else finalColor = newColor;
			finalColor = finalColor & colorWriteMask;
		*/
	}

	//	- Global color blending settings. Set blend constants that you can use as blend factors in the aforementioned calculations.
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;					// VK_FALSE: Blending method of mixing values.  VK_TRUE: Blending method of bitwise values combination (this disables the previous structure, like blendEnable = VK_FALSE).
	colorBlending.logicOp = VK_LOGIC_OP_COPY;			// Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;						// Optional
	colorBlending.blendConstants[1] = 0.0f;						// Optional
	colorBlending.blendConstants[2] = 0.0f;						// Optional
	colorBlending.blendConstants[3] = 0.0f;						// Optional

	// Dynamic states: A limited amount of the state that we specified in the previous structs can actually be changed without recreating the pipeline (size of viewport, lined width, blend constants...). If you want to do that, you have to fill this struct. This will cause the configuration of these values to be ignored and you will be required to specify the data at drawing time. This struct can be substituted by a nullptr later on if you don't have any dynamic state.
	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Create graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	//pipelineInfo.flags				= VK_PIPELINE_CREATE_DERIVATIVE_BIT;	// Required for using basePipelineHandle and basePipelineIndex members
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;	// [Optional]
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;			// [Optional] <<< NO SE AÑADIÓ LA STRUCT dynamicState
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = e.renderPass;			// <<< It's possible to use other render passes with this pipeline instead of this specific instance, but they have to be compatible with "renderPass" (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#renderpass-compatibility).
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// [Optional] Specify the handle of an existing pipeline.
	pipelineInfo.basePipelineIndex = -1;				// [Optional] Reference another pipeline that is about to be created by index.

	if (vkCreateGraphicsPipelines(e.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline!");
	
	// Cleanup
	//vkDestroyShaderModule(e.device, fragShaderModule, nullptr);
	//vkDestroyShaderModule(e.device, vertShaderModule, nullptr);
}

VkShaderModule ModelData::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());	// The default allocator from std::vector ensures that the data satisfies the alignment requirements of `uint32_t`.

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(e.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module!");

	return shaderModule;
}

// (19)
void ModelData::createVertexBuffer()
{
	// Create a staging buffer (host visible buffer used as temporary buffer for mapping and copying the vertex data) (https://vkguide.dev/docs/chapter-5/memory_transfers/)
	VkDeviceSize   bufferSize = vertices.totalBytes();	// sizeof(vertices[0])* vertices.size();
	VkBuffer	   stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(
		e,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 											// VK_BUFFER_USAGE_ ... TRANSFER_SRC_BIT / TRANSFER_DST_BIT (buffer can be used as source/destination in a memory transfer operation).
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// Fill the staging buffer (by mapping the buffer memory into CPU accessible memory: https://en.wikipedia.org/wiki/Memory-mapped_I/O)
	void* data;
	vkMapMemory(e.device, stagingBufferMemory, 0, bufferSize, 0, &data);	// Access a memory region. Use VK_WHOLE_SIZE to map all of the memory.
	memcpy(data, vertices.data(), (size_t)bufferSize);						// Copy the vertex data to the mapped memory.
	vkUnmapMemory(e.device, stagingBufferMemory);							// Unmap memory.

	/*
		Note:
		The driver may not immediately copy the data into the buffer memory (example: because of caching).
		It is also possible that writes to the buffer are not visible in the mapped memory yet. Two ways to deal with that problem:
		  - (Our option) Coherent memory heap: Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT. This ensures that the mapped memory always matches the contents of the allocated memory (this may lead to slightly worse performance than explicit flushing, but this doesn't matter since we will use a staging buffer).
		  - Flushing memory: Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges before reading from the mapped memory.
		Either option means that the driver will be aware of our writes to the buffer, but it doesn't mean that they are actually visible on the GPU yet.
		The transfer of data to the GPU happens in the background and the specification simply tells us that it is guaranteed to be complete as of the next call to vkQueueSubmit.
	*/

	// Create the actual vertex buffer (Device local buffer used as actual vertex buffer. Generally it doesn't allow to use vkMapMemory, but we can copy from stagingBuffer to vertexBuffer, though you need to specify the transfer source flag for stagingBuffer and the transfer destination flag for vertexBuffer).
	// This makes vertex data to be loaded from high performance memory.
	createBuffer(
		e,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory);

	// Move the vertex data to the device local buffer
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	// Clean up
	vkDestroyBuffer(e.device, stagingBuffer, nullptr);
	vkFreeMemory(e.device, stagingBufferMemory, nullptr);

	//std::cout << "Vertex count: " << bufferSize/32 << std::endl;
	//for (size_t i = 0; i < bufferSize/32; i++)
	//	std::cout << i << ": " << ((float*)(vertices.data()))[i * 8 + 0] << ", " << ((float*)(vertices.data()))[i * 8 + 1] << ", " << ((float*)(vertices.data()))[i * 8 + 2] << ", " << ((float*)(vertices.data()))[i * 8 + 3] << ", " << ((float*)(vertices.data()))[i * 8 + 4] << ", " << ((float*)(vertices.data()))[i * 8 + 5] << ", " << ((float*)(vertices.data()))[i * 8 + 6] << ", " << ((float*)(vertices.data()))[i * 8 + 7] << std::endl;
}

/**
	@brief Copies some amount of data (size) from srcBuffer into dstBuffer. Used in createVertexBuffer() and createIndexBuffer().

	Memory transfer operations are executed using command buffers (like drawing commands), so we allocate a temporary command buffer. You may wish to create a separate command pool for these kinds of short-lived buffers, because the implementation could apply memory allocation optimizations. You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag during command pool generation in that case.
*/
void ModelData::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = e.beginSingleTimeCommands();

	// Specify buffers and the size of the contents you will transfer (it's not possible to specify VK_WHOLE_SIZE here, unlike vkMapMemory command).
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	copyRegion.srcOffset = 0;	// Optional
	copyRegion.dstOffset = 0;	// Optional

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	e.endSingleTimeCommands(commandBuffer);
}

// (20)
void ModelData::createIndexBuffer()
{
	// Create a staging buffer
	VkDeviceSize   bufferSize = sizeof(indices[0]) * indices.size();
	VkBuffer	   stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(
		e,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// Fill the staging buffer
	void* data;
	vkMapMemory(e.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(e.device, stagingBufferMemory);

	// Create the vertex buffer
	createBuffer(
		e,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory);

	// Move the vertex data to the device local buffer
	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	// Clean up
	vkDestroyBuffer(e.device, stagingBuffer, nullptr);
	vkFreeMemory(e.device, stagingBufferMemory, nullptr);
}

// (22)
void ModelData::createDescriptorPool()
{
	// Describe our descriptor sets.
	std::vector<VkDescriptorPoolSize> poolSizes;
	VkDescriptorPoolSize pool;

	if (vsDynUBO.range)
	{
		pool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;					// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
		pool.descriptorCount = static_cast<uint32_t>(e.swapChainImages.size());	// Number of descriptors of this type to allocate
		poolSizes.push_back(pool);
	}

	if (fsUBO.range)
	{
		pool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool.descriptorCount = static_cast<uint32_t>(e.swapChainImages.size());
		poolSizes.push_back(pool);
	}

	for (size_t i = 0; i < textures.size(); ++i)
	{
		pool.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool.descriptorCount = static_cast<uint32_t>(e.swapChainImages.size());
		poolSizes.push_back(pool);
	}

	// Allocate one of these descriptors for every frame.
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(e.swapChainImages.size());	// Max. number of individual descriptor sets that may be allocated
	poolInfo.flags = 0;													// Determine if individual descriptor sets can be freed (VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) or not (0). Since we aren't touching the descriptor set after its creation, we put 0 (default).

	if (vkCreateDescriptorPool(e.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool!");
}

// (23)
void ModelData::createDescriptorSets()
{
	descriptorSets.resize(e.swapChainImages.size());

	std::vector<VkDescriptorSetLayout> layouts(e.swapChainImages.size(), descriptorSetLayout);

	// Describe the descriptor set. Here, we will create one descriptor set for each swap chain image, all with the same layout
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;										// Descriptor pool to allocate from
	allocInfo.descriptorSetCount = static_cast<uint32_t>(e.swapChainImages.size());	// Number of descriptor sets to allocate
	allocInfo.pSetLayouts = layouts.data();											// Descriptor layout to base them on

	// Allocate the descriptor set handles
	if (vkAllocateDescriptorSets(e.device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets!");

	// Populate each descriptor set.
	for (size_t i = 0; i < e.swapChainImages.size(); i++)
	{
		VkDescriptorBufferInfo bufferInfo_vs{};
		if (vsDynUBO.range) bufferInfo_vs.buffer = vsDynUBO.uniformBuffers[i];
		bufferInfo_vs.offset = 0;
		bufferInfo_vs.range = vsDynUBO.range;

		VkDescriptorBufferInfo bufferInfo_fs{};
		if(fsUBO.range) bufferInfo_fs.buffer = fsUBO.uniformBuffers[i];
		bufferInfo_fs.offset = 0;
		bufferInfo_fs.range = fsUBO.range;

		//VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo[textures.size()];
		std::vector<VkDescriptorImageInfo> imageInfo(textures.size());
		for (size_t i = 0; i < textures.size(); i++) {
			imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo[i].imageView = textures[i]->textureImageView;
			imageInfo[i].sampler = textures[i]->textureSampler;
		}
		
		std::vector<VkWriteDescriptorSet> descriptorWrites;
		VkWriteDescriptorSet descriptor;
		uint32_t binding = 0;

		if (vsDynUBO.range)
		{
			descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor.dstSet = descriptorSets[i];
			descriptor.dstBinding = binding++;
			descriptor.dstArrayElement = 0;
			descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			descriptor.descriptorCount = 1;
			descriptor.pBufferInfo = &bufferInfo_vs;
			descriptor.pImageInfo = nullptr;
			descriptor.pTexelBufferView = nullptr;
			descriptor.pNext = nullptr;

			descriptorWrites.push_back(descriptor);
		}

		if (fsUBO.range)
		{
			descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor.dstSet = descriptorSets[i];
			descriptor.dstBinding = binding++;
			descriptor.dstArrayElement = 0;
			descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor.descriptorCount = 1;
			descriptor.pBufferInfo = &bufferInfo_fs;
			descriptor.pImageInfo = nullptr;
			descriptor.pTexelBufferView = nullptr;
			descriptor.pNext = nullptr;

			descriptorWrites.push_back(descriptor);
		}

		if (textures.size())
		{
			descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor.dstSet = descriptorSets[i];
			descriptor.dstBinding = binding++;
			descriptor.dstArrayElement = 0;
			descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor.descriptorCount = textures.size();			// LOOK maybe this can be used instead of the for-loop
			descriptor.pBufferInfo = nullptr;
			descriptor.pImageInfo = imageInfo.data();
			descriptor.pTexelBufferView = nullptr;
			descriptor.pNext = nullptr;

			descriptorWrites.push_back(descriptor);
		}
		
		vkUpdateDescriptorSets(e.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);	// Accepts 2 kinds of arrays as parameters: VkWriteDescriptorSet, VkCopyDescriptorSet.
	}
}

void ModelData::recreateSwapChain()
{
	createGraphicsPipeline();			// Recreate graphics pipeline because viewport and scissor rectangle size is specified during graphics pipeline creation (this can be avoided by using dynamic state for the viewport and scissor rectangles).

	vsDynUBO.createUniformBuffers();	// Uniform buffers depend on the number of swap chain images.
	fsUBO.createUniformBuffers();
	createDescriptorPool();				// Descriptor pool depends on the swap chain images.
	createDescriptorSets();				// Descriptor sets
}

void ModelData::cleanupSwapChain()
{
	// Graphics pipeline
	vkDestroyPipeline(e.device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(e.device, pipelineLayout, nullptr);

	// Uniform buffers & memory
	vsDynUBO.destroyUniformBuffers();
	fsUBO.destroyUniformBuffers();

	// Descriptor pool & Descriptor set (When a descriptor pool is destroyed, all descriptor-sets allocated from the pool are implicitly/automatically freed and become invalid)
	vkDestroyDescriptorPool(e.device, descriptorPool, nullptr);
}

void ModelData::cleanup()
{
	// Descriptor set layout
	vkDestroyDescriptorSetLayout(e.device, descriptorSetLayout, nullptr);
	
	// Index
	if (indices.size())
	{
		vkDestroyBuffer(e.device, indexBuffer, nullptr);
		vkFreeMemory(e.device, indexBufferMemory, nullptr);
	}

	// Vertex
	vkDestroyBuffer(e.device, vertexBuffer, nullptr);
	vkFreeMemory(e.device, vertexBufferMemory, nullptr);

}

void ModelData::setRenderCount(size_t numRenders)
{
	this->activeRenders = numRenders;

	if (numRenders > vsDynUBO.numDynUBOs)
	{
		vsDynUBO.resizeUBO(numRenders);

		if (fullyConstructed)
		{
			vsDynUBO.destroyUniformBuffers();
			vkDestroyDescriptorPool(e.device, descriptorPool, nullptr);	// Descriptor-Sets are automatically freed when the descriptor pool is destroyed.

			vsDynUBO.createUniformBuffers();	// Create a UBO with the new size
			createDescriptorPool();				// Required for creating descriptor sets
			createDescriptorSets();				// Contains the UBO
		}
	}
}
