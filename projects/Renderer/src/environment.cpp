
#include <stdexcept>
#include <iostream>
//#include <memory>				// std::unique_ptr, std::shared_ptr (used instead of RAII)
#include <map>					// std::multimap<key, value>
#include <set>					// std::set<uint32_t>
#include <array>
//#include <cstring>			// strcmp()

#include "environment.hpp"


bool QueueFamilyIndices::isComplete()
{
	return	graphicsFamily.has_value() && presentFamily.has_value();
}

Image::Image() : image(nullptr), memory(nullptr), view(nullptr), sampler(nullptr) { }

void Image::destroy(VulkanEnvironment* e)
{
	if(view)    vkDestroyImageView(e->c.device, view, nullptr);		// Resolve buffer	(VkImageView)
	if(image)   vkDestroyImage(e->c.device, image, nullptr);			// Resolve buffer	(VkImage)
	if(memory)  vkFreeMemory(e->c.device, memory, nullptr);			// Resolve buffer	(VkDeviceMemory)
	e->c.memAllocObjects--;
	if(sampler) vkDestroySampler(e->c.device, sampler, nullptr);
}

SwapChain::SwapChain()
	: swapChain(nullptr), imageFormat(VK_FORMAT_UNDEFINED), extent(VkExtent2D{0,0}) { }

void SwapChain::destroy(VkDevice device)
{
	// Swap chain image views
	for (auto imageView : views)
		vkDestroyImageView(device, imageView, nullptr);

	// Swap chain
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

//VkSwapchainKHR							swapChain;				//!< Swap chain object.
//std::vector<VkImage>						swapChainImages;		//!< List. Opaque handle to an image object.
//std::vector<VkImageView>					swapChainImageViews;	//!< List. Opaque handle to an image view object. It allows to use VkImage in the render pipeline. It's a view into an image; it describes how to access the image and which part of the image to access.
//std::vector<std::array<VkFramebuffer, 2>>	swapChainFramebuffers;	//!< List. Opaque handle to a framebuffer object (set of attachments, including the final image to render). Access: swapChainFramebuffers[numSwapChainImages][attachment]. First attachment: main color. Second attachment: post-processing
//
//VkFormat									swapChainImageFormat;
//VkExtent2D									swapChainExtent;

VulkanCore::VulkanCore(IOmanager& io)
	: physicalDevice(VK_NULL_HANDLE), msaaSamples(VK_SAMPLE_COUNT_1_BIT), io(io), memAllocObjects(0)
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	//initWindow();
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();

	minUniformBufferOffsetAlignment = getMinUniformBufferOffsetAlignment();
	supportsAF = supportsAnisotropicFiltering();
}

VulkanEnvironment::VulkanEnvironment(IOmanager& io)
	: c(io), io(io), inputAttachmentCount(2)
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	createSwapChain();
	createSwapChainImageViews();

	createCommandPool();

	createRenderPass();
	createImageResources();
	createFramebuffers();
}

VulkanEnvironment::~VulkanEnvironment() 
{ 
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif
}

void VulkanCore::initWindow()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif
/*
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	// Tell GLFW not to create an OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);		// Enable resizable window (default)

	window = glfwCreateWindow((int)width, (int)height, "Grapho", nullptr, nullptr);
	//glfwSetWindowUserPointer(window, this);								// Input class has been set as windowUserPointer
	//glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);	// This callback has been set in Input

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
	*/
}

// (1) 
/// Describe application, select extensions and validation layers, create Vulkan instance (stores application state).
void VulkanCore::createInstance()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	// Check validation layer support
	if (enableValidationLayers && !checkValidationLayerSupport(requiredValidationLayers))
		throw std::runtime_error("Validation layers requested, but not available!");

	// [Optional] Tell the compiler some info about the instance to create (used for optimization)
	VkApplicationInfo appInfo{};

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Graphox";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;
	appInfo.pNext = nullptr;					// pointer to extension information

	// Not optional. Tell the compiler the global extensions and validation layers we will use (applicable to the entire program, not a specific device)
	VkInstanceCreateInfo createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers)
	{
		createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
		createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	auto extensions = getRequiredExtensions();
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

	// Check for extension support
	if (!checkExtensionSupport(createInfo.ppEnabledExtensionNames, createInfo.enabledExtensionCount))
		throw std::runtime_error("Extensions requested, but not available!");

	// Create the instance
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create instance!");
}

bool VulkanCore::checkValidationLayerSupport(const std::vector<const char*>& requiredLayers)
{
	// Number of layers available
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	// Names of the available layers
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// Print "requiredLayers" and "availableLayers"
	#ifdef DEBUG_ENV_INFO
		std::cout << "   Required validation layers: \n";
		for (size_t i = 0; i < requiredLayers.size(); ++i)
			std::cout << "      " << requiredLayers[i] << '\n';

		std::cout << "   Available validation layers: \n";
		for (size_t i = 0; i < layerCount; ++i)
			std::cout << "      " << availableLayers[i].layerName << '\n';
	#endif

	// Check if all the "requiredLayers" exist in "availableLayers"
	for (const char* reqLayer : requiredLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (std::strcmp(reqLayer, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			return false;		// If any layer is not found, returns false
	}

	return true;				// If all layers are found, returns true
}

/**
*	@brief Specify the details about the messenger and its callback.
* 
*	Specify the details about the messenger and its callback
*	There are a lot more settings for the behavior of validation layers than just the flags specified in the
*	VkDebugUtilsMessengerCreateInfoEXT struct. The file "$VULKAN_SDK/Config/vk_layer_settings.txt" explains how to configure the layers.
*	@param createInfo Struct that this method will use for setting the type of messages to receive, and the callback function.
 */
void VulkanCore::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	// - Type of the struct
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	// - Specify the types of severities you would like your callback to be called for.
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	// - Specify the types of messages your callback is notified about.
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	// - Specify the pointer to the callback function.
	createInfo.pfnUserCallback = debugCallback;
	// - [Optional] Pass a pointer to the callback function through this parameter
	createInfo.pUserData = nullptr;
}

/**
	@brief Callback for handling ourselves the validation layer's debug messages and decide which kind of messages to see.

   The validation layers will print debug messages to the standard output by default.
   But by providing a callback we can handle them ourselves and decide which kind of messages to see.
   This callback function is added with the PFN_vkDebugUtilsMessengerCallbackEXT prototype.
   The VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it.
   @param messageSeverity Specifies the severity of the message, which is one of the following flags (it's possible to use comparison operations between them):
		<ul>
			<li>VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message.</li>
			<li>VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message (such as the creation of a resource).</li>
			<li>VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior (not necessarily an error, but very likely a bug).</li>
			<li>VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior that is invalid and may cause crashes.</li>
		</ul>
   @param messageType Can have the following values:
		<ul>
			<li>VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event happened that is unrelated to the specification or performance.</li>
			<li>VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something happened that violates the specification or indicates a possible mistake.</li>
			<li>VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan.</li>
		</ul>
   @param pCallbackData It refers to a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the message. Some important members:
		<ul>
			<li>pMessage: Debug message as null-terminated string.</li>
			<li>pObjects: Array of Vulkan object handles related to the message.</li>
			<li>objectCount: Number of objects in the array.</li>
		</ul>
   @param pUserData Pointer (specified during the setup of the callback) that allows you to pass your own data.
   @return Boolean indicating if the Vulkan call that triggered the validation layer message should be aborted. If true, the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error.
 */
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanCore::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	//std::cerr << "Validation layer: (" << messageSeverity << '/' << messageType << ") " << pCallbackData->pMessage << std::endl;
	if (messageType != 1)			// Avoid GENERAL_BIT messages (unrelated to the specification or performance).
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

/// Get a list of required extensions (based on whether validation layers are enabled or not)
std::vector<const char*> VulkanCore::getRequiredExtensions()
{
	// Get required extensions (glfwExtensions)
	const char** glfwExtensions;
	uint32_t glfwExtensionCount = 0;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Store them in a vector
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	// Add additional optional extensions

	// > VK_EXT_DEBUG_UTILS_EXTENSION_NAME == "VK_EXT_debug_utils". 
	// This extension is needed, together with a debug messenger, to set up a callback to handle messages and associated details
	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

bool VulkanCore::checkExtensionSupport(const char* const* requiredExtensions, uint32_t reqExtCount)
{
	// Number of extensions available
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// Names of the available extensions
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	// Print "requiredExtensions" and "availableExtensions"
	#ifdef DEBUG_ENV_INFO
		std::cout << "   Required extensions: \n";
		for (size_t i = 0; i < reqExtCount; ++i)
			std::cout << "      " << requiredExtensions[i] << '\n';

		std::cout << "   Available extensions: \n";
		for (size_t i = 0; i < extensionCount; ++i)
			std::cout << "      " << availableExtensions[i].extensionName << '\n';
	#endif

	// Check if all the "requiredExtensions" exist in "availableExtensions"
	for (size_t i = 0; i < reqExtCount; ++i)
	{
		bool extensionFound = false;
		for (const auto& extensionProperties : availableExtensions)
		{
			if (std::strcmp(requiredExtensions[i], extensionProperties.extensionName) == 0)
			{
				extensionFound = true;
				break;
			}
		}
		if (!extensionFound)
			return false;		// If any extension is not found, returns false
	}

	return true;				// If all extensions are found, returns true
}

// (2)
/**
	@brief Specify the details about the messenger and its callback, and create the debug messenger.

	Specify the details about the messenger and its callback (there are more ways to configure validation layer messages and debug callbacks), and create the debug messenger.
 */
void VulkanCore::setupDebugMessenger()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	if (!enableValidationLayers) return;

	// Fill in a structure with details about the messenger and its callback
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	// Create the debug messenger
	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		throw std::runtime_error("Failed to set up debug messenger!");
}

/**
 * Given a VkDebugUtilsMessengerCreateInfoEXT object, creates/loads the extension object (debug messenger) (VkDebugUtilsMessengerEXT) if it's available.
 * Because it is an extension function, it is not automatically loaded. So, we have to look up its address ourselves using vkGetInstanceProcAddr.
 * @param instance Vulkan instance (the debug messenger is specific to our Vulkan instance and its layers)
 * @param pCreateInfo VkDebugUtilsMessengerCreateInfoEXT object
 * @param pAllocator Optional allocator callback
 * @param pDebugMessenger Debug messenger object
 * @return Returns the extension object, or an error if is not available.
 */
VkResult VulkanCore::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	// Load the extension object if it's available (the extension function needs to be explicitly loaded)
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	// vkGetInstanceProcAddr returns nullptr is the function couldn't be loaded.
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// (3)
/**
	@brief Create a window surface (interface for interacting with the window system)

	Create a window surface (interface for interacting with the window system). Requires to use WSI (Window System Integration), which is provided by GLFW.
 */
void VulkanCore::createSurface()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	io.createWindowSurface(instance, nullptr, &surface);

	//if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	//	throw std::runtime_error("Failed to create window surface!");
}

// (4)
/**
	@brief Look for and select a graphics card in the system that supports the features we need.

	Look for and select a graphics card in the system that supports the features we need (Vulkan support).
 */
void VulkanCore::pickPhysicalDevice()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	// Get all devices with Vulkan support.

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	else
	{
		#ifdef DEBUG_ENV_INFO
			std::cout << "   Devices with Vulkan support: " << deviceCount << std::endl;
		#endif
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	// Look for the most suitable device and select it.

	std::multimap<int, VkPhysicalDevice> candidates;	// Automatically sorts candidates by score

	for (const auto& device : devices)					// Rate each device
	{
		int score = evaluateDevice(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0)					// Check if the best candidate has score > 0
	{
		physicalDevice = candidates.rbegin()->second;
		msaaSamples = getMaxUsableSampleCount(!add_MSAA);
	}
	else
		throw std::runtime_error("Failed to find a suitable GPU!");

	#ifdef DEBUG_ENV_INFO
		std::cout << "   Device features: " << std::endl;
		std::cout << "      MSAA samples: " << msaaSamples << std::endl;
		std::cout << "      Anisotropic filtering: " << (supportsAnisotropicFiltering() ? "Yes" : "No") << std::endl;
		std::cout << "      Minimum uniform buffer offset alignment: " << getMinUniformBufferOffsetAlignment() << std::endl;
		std::cout << "      Large points supported: " << (largePointsSupported() ? "Yes" : "No") << std::endl;
		std::cout << "      Wide lines supported: " << (wideLinesSupported() ? "Yes" : "No") << std::endl;
	#endif
}

/**
*	@brief Evaluate a device and check if it is suitable for the operations we want to perform.
*
*	Evaluate a device and check if it is suitable for the operations we want to perform.
*	@param device Device to evaluate
*	@param mode Mode of evaluation:
*	@return Score for the device (0 means that the device is not suitable). Dedicated GPUs supporting geometry shaders are favored.
*/
int VulkanCore::evaluateDevice(VkPhysicalDevice device)
{
	// Get basic device properties: Name, type, supported Vulkan version...
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	
	// Get optional features: Texture compression, 64 bit floats, multi-viewport rendering...
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// Get queue families
	QueueFamilyIndices indices = findQueueFamilies(device);

	// Check whether required device extensions are supported 
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	// Check whether swap chain extension is compatible with the window surface (adequately supported)
	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();	// Adequate if there's at least one supported image format and one supported presentation mode.
	}

	// Score the device
	int score = 0;

	if (!indices.isComplete()) return 0;				// The queue families we want should exist (Computer graphics & Presentation to window surface).
	if (!extensionsSupported) return 0;					// The required device extensions should be supported.
	if (!swapChainAdequate) return 0;					// Swap chain extension support should be adequate (compatible with window surface).
	if (!deviceFeatures.geometryShader) return 0;		// Applications cannot function without geometry shaders.
	score += 1;

	if (deviceFeatures.samplerAnisotropy) score += 1;	// Anisotropic filtering
	if (deviceFeatures.largePoints) score += 1;
	if (deviceFeatures.wideLines) score += 1;
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;	// Discrete GPUs have better performance (and integrated GPUs, worse).
	score += deviceProperties.limits.maxImageDimension2D;									// Maximum size of textures.
	
	#ifdef DEBUG_ENV_INFO
		std::cout << "      (" << score << ") " << deviceProperties.deviceName << std::endl;
	#endif

	return score;
}

/**
 * Check which queue families are supported by the device and which one of these supports the commands that we want to use (in this case, graphics commands).
 * Queue families: Any operation (drawing, uploading textures...) requires commands commands to be submitted to a queue. There are different types of queues that originate from different queue families and each family of queues allows only a subset of commands (graphics commands, compute commands, memory transfer related commands...).
 * @param device Device to evaluate
 * @return Structure containing vector indices of the queue families we want.
 */
QueueFamilyIndices VulkanCore::findQueueFamilies(VkPhysicalDevice device)
{
	// Get queue families
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		// Check queue families capable of presenting to our window surface
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) indices.presentFamily = i;

		// Check queue families capable of computer graphics
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		if (indices.isComplete()) break;
		i++;
	}

	return indices;
}

QueueFamilyIndices VulkanCore::findQueueFamilies() { return findQueueFamilies(physicalDevice); }

uint32_t VulkanCore::getMaxMemoryAllocationCount()
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	return deviceProperties.limits.maxMemoryAllocationCount;
}

void VulkanCore::destroy()
{
	vkDestroyDevice(device, nullptr);										// Logical device & device queues

	if (enableValidationLayers)												// Debug messenger
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);						// Surface KHR
	vkDestroyInstance(instance, nullptr);									// Instance
	io.destroy();
}

//IOmanager* VulkanCore::getWindowManager() { return &io; }

/**
	Check whether all the required device extensions are supported.
	@param device Device to evaluate
	@return True if all the required device extensions are supported. False otherwise.
*/
bool VulkanCore::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

SwapChainSupportDetails VulkanCore::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	// Get the basic surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	// Get the supported surface formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	// Get supported presentation modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

SwapChainSupportDetails VulkanCore::querySwapChainSupport() { return querySwapChainSupport(physicalDevice); }

/// Get the maximum number of samples (for MSAA) according to the physical device.
VkSampleCountFlagBits VulkanCore::getMaxUsableSampleCount(bool getMinimum)
{
	if (getMinimum) return VK_SAMPLE_COUNT_1_BIT;

	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT ) { return VK_SAMPLE_COUNT_8_BIT;  }
	if (counts & VK_SAMPLE_COUNT_4_BIT ) { return VK_SAMPLE_COUNT_4_BIT;  }
	if (counts & VK_SAMPLE_COUNT_2_BIT ) { return VK_SAMPLE_COUNT_2_BIT;  }
	return VK_SAMPLE_COUNT_1_BIT;
}

// (5)
/// Set up a logical device (describes the features we want to use) to interface with the physical device.
void VulkanCore::createLogicalDevice()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	// Get the queue families supported by the physical device.
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	// Describe the number of queues you want for each queue family
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	float queuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;		// You can assign priorities to queues to influence the scheduling of command buffer execution using floats in the range [0.0, 1.0]. This is required even if there is only a single queue.

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Enable the features from the physical device that you will use (geometry shaders...)
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;								// Anisotropic filtering is an optional device feature (most modern graphics cards support it, but we should check it in isDeviceSuitable)
	deviceFeatures.sampleRateShading = (add_SS ? VK_TRUE : VK_FALSE);		// Enable sample shading feature for the device
	deviceFeatures.wideLines = (wideLinesSupported() ? VK_TRUE : VK_FALSE);	// Enable line width configuration (in VkPipeline)

	// Describe queue parameters
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
	}
	else
		createInfo.enabledLayerCount = 0;

	// Create the logical device
	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		throw std::runtime_error("Failed to create logical device!");

	// Retrieve queue handles for each queue family (in this case, we created a single queue from each family, so we simply use index 0)
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

// (6)
/// Set up and create the swap chain.
void VulkanEnvironment::createSwapChain()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	// Get some properties
	SwapChainSupportDetails swapChainSupport = c.querySwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);	// Surface formats (pixel format, color space)
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);	// Presentation modes
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);		// Basic surface capabilities

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;		// How many images in the swap chain? We choose the minimum required + 1 (this way, we won't have to wait sometimes on the driver to complete internal operations before we can acquire another image to render to.

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)	// Don't exceed max. number of images (if maxImageCount == 0, there is no maximum)
		imageCount = swapChainSupport.capabilities.maxImageCount;

	// Configure the swap chain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = c.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;								// Number of layers each image consists of (always 1, except for stereoscopic 3D applications)
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	// Kind of operations we'll use the images in the swap chain for. VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT let us render directly to the swap chain. VK_IMAGE_USAGE_TRANSFER_DST_BIT let us render images to a separate image ifrst to perform operations like post-processing and use memory operation to transfer the rendered image to a swap chain image. 

	QueueFamilyIndices indices = c.findQueueFamilies();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)			// Specify how to handle swap chain images that will be used across multiple queue families. This will be the case if the graphics queue family is different from the presentation queue (draws on the images in the swap chain from the graphics queue and submits them on the presentation queue).
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;	// Best performance. An image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family.
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;	// Images can be used across multiple queue families without explicit ownership transfers.
		createInfo.queueFamilyIndexCount = 0;						// Optional
		createInfo.pQueueFamilyIndices = nullptr;					// Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;	// currentTransform specifies that you don't want any transformation ot be applied to images in the swap chain.
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				// Specify if the alpha channel should be used for blending with other windows in the window system. VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR makes it ignore the alpha channel.
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;												// If VK_TRUE, we don't care about colors of pixels that are obscured (example, because another window is in front of them).
	createInfo.oldSwapchain = VK_NULL_HANDLE;									// It's possible that your swap chain becomes invalid/unoptimized while the application is running (example: window resize), so your swap chain will need to be recreated from scratch and a reference to the old one must be specified in this field.

	// Create swap chain
	if (vkCreateSwapchainKHR(c.device, &createInfo, nullptr, &swapChain.swapChain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create swap chain!");

	// Retrieve the handles
	vkGetSwapchainImagesKHR(c.device, swapChain.swapChain, &imageCount, nullptr);
	swapChain.images.resize(imageCount);
	vkGetSwapchainImagesKHR(c.device, swapChain.swapChain, &imageCount, swapChain.images.data());

	#ifdef DEBUG_ENV_INFO
		std::cout << "   Swap chain images: " << swapChain.images.size() << std::endl;
	#endif

	// Save format and extent for future use
	swapChain.imageFormat = surfaceFormat.format;
	swapChain.extent = extent;
}

/// Chooses the surface format (color depth) for the swap chain.
VkSurfaceFormatKHR VulkanEnvironment::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// Return our favourite surface format, if it exists
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&				// Format: Color channel and types (example: VK_FORMAT_B8G8R8A8_SRGB is BGRA channels with 8 bit unsigned integer)
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)	// Color space: Indicates if the sRGB color space is supported or not (https://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colors-in-a-linear-vs-a-no) (https://stackoverflow.com/questions/66401081/vulkan-swapchain-format-unorm-vs-srgb).
			return availableFormat;
	}
	
	// Otherwise, return the first format founded (other ways: rank the available formats on how "good" they are)
	std::cout << "Favorite surface format not found" << std::endl;
	return availableFormats[0];
}

/// Chooses the swap extent (resolution of images in swap chain) for the swap chain.
/** The swap extent is set here. The swap extent is the resolution (in pixels) of the swap chain images, which is almost always equal to the resolution of the window where we are drawing (use {WIDHT, HEIGHT}), except when you're using a high DPI display (then, use glfwGetFramebufferSize). */
VkExtent2D VulkanEnvironment::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// If width and height is set to the maximum value of UINT32_MAX, it indicates that the surface size will be determined by the extent of a swapchain targeting the surface. 
	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;

	// Set width and height (useful when you're using a high DPI display)
	else
	{
		int width, height;
		io.getFramebufferSize(&width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

/**
*	@brief Chooses the presentation mode (conditions for "swapping" images to the screen) for the swap chain.
* 
* Presentation mode represents the conditions for showing images to the screen. Four possible modes available in Vulkan:
* 		<ul>
			<li>VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away (may cause tearing).</li>
			<li>VK_PRESENT_MODE_FIFO_KHR: The swap chain is a FIFO queue. The display takes images from the front. The program inserts images at the back. This is most similar to vertical sync as found in modern games. This is the only mode guaranteed to be available.</li>
			<li>VK_PRESENT_MODE_FIFO_RELAXED_KHR: Like the second mode, with one more property: If the application is late and the queue was empty at the last vertical blank (moment when the display is refreshed), instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives (may cause tearing).</li>
			<li>VK_PRESENT_MODE_MAILBOX_KHR: Like the second mode, but instead of blocking the application when the queue is full, the images are replaced with the newer ones. This can be used to implement triple buffering, avoiding tearing with much less latency issues than standard vertical sync that uses double buffering.</li>
		</ul>
	This functions will choose VK_PRESENT_MODE_MAILBOX_KHR if available. Otherwise, it will choose VK_PRESENT_MODE_FIFO_KHR.
*/
VkPresentModeKHR VulkanEnvironment::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	// Choose VK_PRESENT_MODE_MAILBOX_KHR if available
	for (const auto& mode : availablePresentModes)
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;

	// Otherwise, choose VK_PRESENT_MODE_FIFO_KHR
	return VK_PRESENT_MODE_FIFO_KHR;
}

// (7)
/// Creates a basic image view for every image in the swap chain so that we can use them as color targets later on.
void VulkanEnvironment::createSwapChainImageViews()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	swapChain.views.resize(swapChain.images.size());

	for (uint32_t i = 0; i < swapChain.images.size(); i++)
		swapChain.views[i] = createImageView(swapChain.images[i], swapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VulkanEnvironment::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	// Create image objects for letting the shader access the pixel values (better option than setting up the shader to access the pixel values in the buffer). Pixels within an image object are known as texels.
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;				// Kind of coordinate system the texels in the image are going to be addressed: 1D (to store an array of data or gradient...), 2D (textures...), 3D (to store voxel volumes...).
	imageInfo.extent.width = width;						// Number of texels in X									
	imageInfo.extent.height = height;					// Number of texels in Y
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;					// Number of levels (mipmaps)
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;							// Same format for the texels as the pixels in the buffer. This format is widespread, but if it is not supported by the graphics hardware, you should go with the best supported alternative.
	imageInfo.tiling = tiling;							// This cannot be changed later. VK_IMAGE_TILING_ ... LINEAR (texels are laid out in row-major order like our pixels array), OPTIMAL (texels are laid out in an implementation defined order for optimal access).
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;// VK_IMAGE_LAYOUT_ ... UNDEFINED (not usable by the GPU and the very first transition will discard the texels), PREINITIALIZED (not usable by the GPU, but the first transition will preserve the texels). We choose UNDEFINED because we're first going to transition the image to be a transfer destination and then copy texel data to it from a buffer object. There are few situations where PREINITIALIZED is necessary (example: when we want to use an image as a staging image in combination with the VK_IMAGE_TILING_LINEAR layout). 
	imageInfo.usage = usage;							// The image is going to be used as destination for the buffer copy, so it should be set up as a transfer destination; and we also want to be able to access the image from the shader to color our mesh.
	imageInfo.samples = numSamples;						// For multisampling. Only relevant for images that will be used as attachments.
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// The image will only be used by one queue family: the one that supports graphics (and therefore also) transfer operations.
	imageInfo.flags = 0;								// [Optional]  There are some optional flags for images that are related to sparse images (images where only certain regions are actually backed by memory). Example: If you were using a 3D texture for a voxel terrain, then you could use this to avoid allocating memory to store large volumes of "air" values.

	if (vkCreateImage(c.device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image!");

	// Allocate memory for the image
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(c.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(c.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate image memory!");

	c.memAllocObjects++;

	vkBindImageMemory(c.device, image, imageMemory, 0);
}

VkImageView VulkanEnvironment::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;			// 1D, 2D, 3D, or cube map
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;						// Image's purpose and which part of it should be accessed. Here, our images will be used as color targets without any mipmapping levels or multiple layers. 
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	// Default color mapping
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Note about stereographic 3D applications: For them, you would create a swap chain with multiple layers, and then create multiple image views for each image (one for left eye and another for right eye).

	VkImageView imageView;
	if (vkCreateImageView(c.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture image view!");

	return imageView;
}

/// Find the right format for a depth image.
/** Select a format with a depth component that supports usage as depth attachment. We don't need a specific format because we won't be directly accessing the texels from the program. It just needs to have a reasonable accuracy (usually, at least 24 bits). Several formats fit this requirement: VK_FORMAT_ ... D32_SFLOAT (32-bit signed float depth), D32_SFLOAT_S8_UINT (32-bit signed float depth and 8 bit stencil), D24_UNORM_S8_UINT (24-bit float depth and 8 bit stencil). */
VkFormat VulkanEnvironment::findDepthFormat()
{
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

//IOmanager* VulkanEnvironment::getWindowManager() { return c.getWindowManager(); }

/**
* 	@brief Finds the right type of memory to use, depending upon the requirements of the buffer and our own application requiremnts.
*	
*	Graphic cards can offer different types of memory to allocate from. Each type of memory varies in terms of allowed operations and performance characteristics.
*	@param typeFilter Specifies the bit field of memory types that are suitable.
*	@param properties Specifies the bit field of the desired properties of such memory types.
*	@return Index of a memory type suitable for the buffer that also has all of the properties we need.
*/
uint32_t VulkanEnvironment::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	// Query info about the available types of memory.
	VkPhysicalDeviceMemoryProperties memProperties;				// This struct has 2 arrays memoryTypes and memoryHeaps (this one are distinct memory resources, like dedicated VRAM and swap space in RAM for when VRAM runs out). Right now we'll concern with the type of memory and not the heap it comes from.
	vkGetPhysicalDeviceMemoryProperties(c.physicalDevice, &memProperties);

	// Find a memory type suitable for the buffer, and to .
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) &&													// Find the index of a suitable memory type for our buffer by iterating over the memory types and checking if the corresponding bit is set to 1.
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)	// Check that the memory type has some properties.
			return i;
	}

	throw std::runtime_error("Failed to find suitable memory type!");
}

/**
*	@brief Take a list of candidate formats in order from most desirable to least desirable, and check which is the first one that is supported.
* 
*	@param candidates
*	@param tiling The support of a format depends on the tiling mode.
*	@param features The support of a format depends on the usage.
*	@return
*/
VkFormat VulkanEnvironment::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;												// Contains 3 fields: linearTilingFeatures (linear tiling), optimalTilingFeatures (optimal tiling), bufferFeatures (buffers).
		vkGetPhysicalDeviceFormatProperties(c.physicalDevice, format, &props);	// Query the support of a format

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}

	throw std::runtime_error("Failed to find supported format!");
}

// (11) <<<
/// Commands in Vulkan (drawing, memory transfers, etc.) are not executed directly using function calls, you have to record all of the operations you want to perform in command buffer objects. After setting up the drawing commands, just tell Vulkan to execute them in the main loop.
void VulkanEnvironment::createCommandPool()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	QueueFamilyIndices queueFamilyIndices = c.findQueueFamilies();	// <<< wrapped method

	// Command buffers are executed by submitting them on one of the device queues we retrieved (graphics queue, presentation queue, etc.). Each command pool can only allocate command buffers that are submitted on a single type of queue.
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;	// [Optional]  VK_COMMAND_POOL_CREATE_ ... TRANSIENT_BIT (command buffers are rerecorded with new commands very often - may change memory allocation behavior), RESET_COMMAND_BUFFER_BIT (command buffers can be rerecorded individually, instead of reseting all of them together). Not necessary if we just record the command buffers at the beginning of the program and then execute them many times in the main loop.

	if (vkCreateCommandPool(c.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool!");
}

void VulkanEnvironment::createRenderPass()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	if (c.msaaSamples > 1) createRenderPass_MS_PP();	// createRenderPass_2x2()
	else createRenderPass_PP();
}

void VulkanEnvironment::createImageResources()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	if (c.msaaSamples > 1) createImageResources_MS_PP();
	else createImageResources_PP();
}

void VulkanEnvironment::createFramebuffers()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	if (c.msaaSamples > 1) createFramebuffers_MS_PP();
	else createFramebuffers_PP();
}

/**
*	Submit a pipeline barrier. It specifies when a transition happens: when the pipeline finishes (source) and the next one starts (destination). No command may start before it finishes transitioning. Commands come at the top of the pipeline (first stage), shaders are executed in order, and commands retire at the bottom of the pipeline (last stage), when execution finishes. This barrier will wait for everything to finish and block any work from starting.
*/
void VulkanEnvironment::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};			// One of the most common way to perform layout transitions is using an image memory barrier. A pipeline barrier like that is generally used to synchronize access to resources, like ensuring that a write to a buffer completes before reading from it, but it can also be used to transition image layouts and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE is used. There is an equivalent buffer memory barrier to do this for buffers.
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;								// Specify layout transition (it's possible to use VK_IMAGE_LAYOUTR_UNDEFINED if you don't care about the existing contents of the image).
	barrier.newLayout = newLayout;								// Specify layout transition
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;					// If you are using the barrier to transfer queue family ownership, then these two fields ...
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;					// ... should be the indices of the queue families; otherwise, set this to VK_QUEUE_FAMILY_IGNORED.
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;				// subresourceRange specifies the part of the image that is affected.
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;								// If the image has no mipmapping levels, then levelCount = 1
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;										// If the image is not an array, then layerCount = 1

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	// Set the access masks and pipeline stages based on the layouts in the transition. There are 2 transitions we need to handle:
	//		- From somewhere (undefined) to transfer destination: Transfer writes that don't need to wait on anything.
	//		- From transfer destinations to shader reading: Shader reads should wait on transfer writes (specifically the shader reads in the fragment shader, because that's where we're going to use the texture).
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;					// The depth buffer is read from  in the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT stage to perform depth tests 
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;		// The depth buffer is written to in the VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT  stage when a new fragment is drawn
	}
	else
		throw std::invalid_argument("Unsupported layout transition!");

	// Submit a pipeline barrier
	{
		const std::lock_guard<std::mutex> lock(mutCommandPool);
		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage,		// Specify in which pipeline stage the operations occur that should happen before the barrier 
			destinationStage,	// Specify the pipeline stage in which operations will wait on the barrier
			0,					// This is either 0 (nothing) or VK_DEPENDENCY_BY_REGION_BIT (turns the barrier into a per-region condition, which means that the implementation is allowed to already begin reading from the parts of a resource that were written so far, for example).
			0, nullptr,			// Array of pipeline barriers of type memory barriers
			0, nullptr,			// Array of pipeline barriers of type buffer memory barriers
			1, &barrier);		// Array of pipeline barriers of type image memory barriers
	}

	endSingleTimeCommands(commandBuffer);

	/*
		Note:
		The pipeline stages that you are allowed to specify before and after the barrier depend on how you use the resource before and after the barrier.
		Allowed values: https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported
		Example: If you're going to read from a uniform after the barrier, you would specify a usage of VK_ACCESS_UNIFORM_READ_BIT and the earliest shader
		that will read from the uniform as pipeline stage (for example, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT).
		----------
		Transfer writes must occur in the pipeline transfer stage. Since the writes don't have to wait on anything, you may specify an empty access mask and the
		earliest possible pipeline stage VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT for the pre-barrier operations. Note that VK_PIPELINE_STAGE_TRANSFER_BIT is not a real
		stage within the graphics and compute pipelines, but a pseudo-stage where transfers happen (more about pseudo-stages:
		https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPipelineStageFlagBits.html).

		The image will be written in the same pipeline stage and subsequently read by the fragment shader, which is why we specify shader reading access in the
		fragment shader pipeline stage.

		If we need to do more transitions in the future, then we'll extend the function.
		Note that command buffer submission results in implicit VK_ACCESS_HOST_WRITE_BIT synchronization at the beginning. Since this function (transitionImageLayout)
		executes a command buffer with only a single command, you could use this implicit synchronization and set srcAccessMask to 0 if you ever needed a
		VK_ACCESS_HOST_WRITE_BIT dependency in a layout transition. It's up to you if you want to be explicit about it or not, but I prefer not to rely on these
		OpenGL-like "hidden" operations.

		VK_IMAGE_LAYOUT_GENERAL: Special type of image layout that supports all operations, although it doesn't necessarily offer the best performance for any
		operation. It is required for some special cases (using an image as both input and output, reading an image after it has left the preinitialized layout, etc.).

		All of the helper functions that submit commands so far have been set up to execute synchronously by waiting for the queue to become idle. For practical
		applications it is recommended to combine these operations in a single command buffer and execute them asynchronously for higher throughput (especially the
		transitions and copy in the createTextureImage function). Exercise: Try to experiment with this by creating a setupCommandBuffer that the helper functions
		record commands into, and add a flushSetupCommands to execute the commands that have been recorded so far. It's best to do this after the texture mapping works
		to check if the texture resources are still set up correctly.
	*/
}

/// Tells if the chosen depth format contains a stencil component.
bool VulkanEnvironment::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/**
*	Allocate the command buffer and start recording it. Used for a command buffer that will be used only once.
*	Command buffers generated using beginSingleTimeCommands (allocation, start recording) and endSingleTimeCommands (end recording, submit) are freed once its execution completes.
*	@return Returns a Vulkan command buffer object.
*/
VkCommandBuffer VulkanEnvironment::beginSingleTimeCommands()
{
	const std::lock_guard<std::mutex> lock(mutCommandPool);

	// Allocate the command buffer.
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(c.device, &allocInfo, &commandBuffer);

	// Start recording the command buffer.
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;		// Good practice to tell the driver about our intent <<< (see createCommandBuffers > beginInfo.flags)

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

/**
*	Stop recording a command buffer and submit it to the queue. Used together with beginSingleTimeCommands().
*/
void VulkanEnvironment::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	const std::lock_guard<std::mutex> lock(mutCommandPool);

	vkEndCommandBuffer(commandBuffer);		// Stop recording (this command buffer only contains the copy command, so we can stop recording now).

	// Execute the command buffer (only contains the copy command) to complete the transfer of buffers.
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	{
		const std::lock_guard<std::mutex> lock(queueMutex);
		vkQueueSubmit(c.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(c.graphicsQueue);			// Wait to this transfer to complete. Two ways to do this: vkQueueWaitIdle (Wait for the transfer queue to become idle. Execute one transfer at a time) or vkWaitForFences (Use a fence. Allows to schedule multiple transfers simultaneously and wait for all of them complete. It may give the driver more opportunities to optimize).
	}

	// Clean up the command buffer used.
	vkFreeCommandBuffers(c.device, commandPool, 1, &commandBuffer);
}

void VulkanEnvironment::recreate_Images_RenderPass_SwapChain()
{
	createSwapChain();					// Recreate the swap chain.
	createSwapChainImageViews();		// Recreate image views because they are based directly on the swap chain images.
	
	createRenderPass();					// Recreate render pass because it depends on the format of the swap chain images.
	createImageResources();
	createFramebuffers();				// Framebuffers directly depend on the swap chain images.
}

void VulkanEnvironment::cleanup_Images_RenderPass_SwapChain()
{
	// Destroy attachments (images)
	color_1.destroy(this);
	depth.destroy(this);
	color_2.destroy(this);

	// Framebuffers
	for (auto framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(c.device, framebuffer[0], nullptr);
		vkDestroyFramebuffer(c.device, framebuffer[1], nullptr);
	}

	// Render pass
	vkDestroyRenderPass(c.device, renderPass[0], nullptr);
	vkDestroyRenderPass(c.device, renderPass[1], nullptr);

	swapChain.destroy(c.device);
}

/**
 * Cleans up the VkDebugUtilsMessengerEXT object.
 * @param instance Vulkan instance (the debug messenger is specific to our Vulkan instance and its layers)
 * @param debugMessenger Debug messenger object
 * @param pAllocator Optional allocator callback
 */
void VulkanCore::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	// Similarly to vkCreateDebugUtilsMessengerEXT, the extension function needs to be explicitly loaded.
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

void VulkanEnvironment::cleanup()
{
	vkDestroyCommandPool(c.device, commandPool, nullptr);
	cleanup_Images_RenderPass_SwapChain();
	c.destroy();
}

// (8)
/*
	@brief Tells Vulkan the framebuffer attachments that will be used while rendering (color, depth, multisampled images). A render-pass denotes more explicitly how your rendering happens.

	Specify subpasses and their attachments.
		- Subpasses: A single render pass can consist of multiple subpasses, which are subsequent rendering operations that depend on the contents of framebuffers in previous passes (example: a sequence of post-processing effects applied one after another). Grouping them into one render pass may give better performance.
		- Attachment references: Every subpass references one or more of the attachments that we've described.
*/
void VulkanEnvironment::createRenderPass_2x2()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	/*
		Process for MultiSampling + PostProcessing:

		Render pass 1:
			Subpass A attachments (background):
				shader -> colorResolveAttachment: Final color (many samplers)
				shader -> depthAttachment[n]: Depth (many samplers)
				X - msaaColorAttachment[n]: Color attachment for multisampling (not used: only 1 sample)
			Subpass B attachments (scene):

		Render pass 2:
			Subpass B attachments:
				shader -> colorResolveAttachment: Final color (many samples)
					colorResolveAttachment: Final color from RP 1 (input attachments)
					depthAttachment: Depth from RP 1 (input attachments)
				msaa -> colorAttachmentPP: Final color (swapChainImageView)
	*/

	// Attachments -------------------------

	// RP 1, SP 1
	VkAttachmentDescription colorAtt_11{};
	VkAttachmentReference colorAttRef_11{};
	VkAttachmentDescription depthAtt_11{};
	VkAttachmentReference depthAttRef_11{};

	// RP 1, SP 2
	VkAttachmentDescription colorAtt_12{};
	VkAttachmentReference colorAttRef_12{};
	VkAttachmentDescription depthAtt_12{};
	VkAttachmentReference depthAttRef_12{};

	// RP 2, Inputs
	VkAttachmentDescription colorInputAtt_1;
	VkAttachmentReference colorInputAttRef_1;
	VkAttachmentDescription depthInputAtt;
	VkAttachmentReference depthInputAttRef;

	// RP2, SP1
	VkAttachmentDescription colorAtt_2{};
	VkAttachmentReference colorAttRef_2{};
	VkAttachmentDescription colorResolveAtt{};
	VkAttachmentReference colorResolveAttRef{};

	VkAttachmentDescription templateAttachment;
	templateAttachment.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
	templateAttachment.format;
	templateAttachment.samples;														// Single color buffer attachment, or many (multisampling).
	templateAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// What to do with the data (color and depth) in the attachment before rendering: VK_ATTACHMENT_LOAD_OP_ ... LOAD (preserve existing contents of the attachment), CLEAR (clear values to a constant at the start of a new frame), DONT_CARE (existing contents are undefined).
	templateAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// What to do with the data (color and depth) in the attachment after rendering:  VK_ATTACHMENT_STORE_OP_ ... STORE (rendered contents will be stored in memory and can be read later), DON_CARE (contents of the framebuffer will be undefined after rendering. We may don't care about it if it will not be used after drawing has finished).
	templateAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// What to do with the stencil data in the attachment before rendering.
	templateAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;			// What to do with the stencil data in the attachment after rendering.
	templateAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// Layout before the render pass. If it's UNDEFINED, we don't care about previous depth contents. Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however the layout of the pixels in memory need to be transitioned to specific layouts suitable for the operation that they're going to be involved in next (read more below).
	templateAttachment.finalLayout;													// Layout to automatically transition after the render pass finishes. VK_IMAGE_LAYOUT_ ... UNDEFINED (we don't care what previous layout the image was in, and the contents of the image are not guaranteed to be preserved), COLOR_ATTACHMENT_OPTIMAL (images used as color attachment), PRESENT_SRC_KHR (images to be presented in the swap chain), TRANSFER_DST_OPTIMAL (Images to be used as destination for a memory copy operation).


	// Color attachment (RP 1, SP 1) (basic multisampling rendering)
	colorAtt_11 = templateAttachment;
	colorAtt_11.format = swapChain.imageFormat;
	colorAtt_11.samples = c.msaaSamples;
	colorAtt_11.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	
	colorAtt_11.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	colorAttRef_11.attachment = 0;											// Specify which attachment to reference by its index in the attachment descriptions array.
	colorAttRef_11.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;		// Specify the layout we would like the attachment to have during a subpass that uses this reference. The layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL will give us the best performance.

	// Depth attachment (RP 1, SP 1)
	depthAtt_11 = templateAttachment;
	depthAtt_11.format = findDepthFormat();									// Should be same format as the depth image
	depthAtt_11.samples = c.msaaSamples;
	depthAtt_11.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	depthAttRef_11.attachment = 1;
	depthAttRef_11.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	// Color attachment (RP 1, SP 2)
	colorAtt_12 = templateAttachment;
	colorAtt_12.format = swapChain.imageFormat;
	colorAtt_12.samples = c.msaaSamples;
	colorAtt_12.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAtt_12.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	colorAttRef_12.attachment = 2;
	colorAttRef_12.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment (RP 1, SP 2)
	depthAtt_12 = templateAttachment;
	depthAtt_12.format = findDepthFormat();
	depthAtt_12.samples = c.msaaSamples;
	depthAtt_12.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	depthAttRef_12.attachment = 3;
	depthAttRef_12.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	// Input attachment (color) (from previous render pass)
	colorInputAtt_1 = templateAttachment;
	colorInputAtt_1.format = swapChain.imageFormat;
	colorInputAtt_1.samples = c.msaaSamples;
	colorInputAtt_1.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	colorInputAttRef_1.attachment = 0;
	colorInputAttRef_1.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Input attachment (depth) (from previous render pass)
	depthInputAtt = templateAttachment;
	depthInputAtt.format = findDepthFormat();
	depthInputAtt.samples = c.msaaSamples;
	depthInputAtt.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	depthInputAttRef.attachment = 1;
	depthInputAttRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Color attachment 2 (Multisample postprocessing) (RP 2)
	colorAtt_2 = templateAttachment;
	colorAtt_2.format = swapChain.imageFormat;
	colorAtt_2.samples = c.msaaSamples;
	colorAtt_2.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	colorAttRef_2.attachment = 2;
	colorAttRef_2.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Final resolved color attachment (RP 2)
	colorResolveAtt = templateAttachment;
	colorResolveAtt.format = swapChain.imageFormat;
	colorResolveAtt.samples = VK_SAMPLE_COUNT_1_BIT;
	colorResolveAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	colorResolveAttRef.attachment = 3;
	colorResolveAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	// Render pass 1 (basic multisample rendering) -------------------------

	VkSubpassDescription subpass_11{};
	subpass_11.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// VK_PIPELINE_BIND_POINT_GRAPHICS: This is a graphics subpass
	subpass_11.colorAttachmentCount = 1;
	subpass_11.pColorAttachments = &colorAttRef_11;						// Attachment for color. The index of the attachment in this array is directly referenced from the fragment shader with the directive "layout(location = 0) out vec4 outColor".
	subpass_11.pResolveAttachments = nullptr;							// Attachments used for resolving multisampling color attachments.
	subpass_11.pDepthStencilAttachment = &depthAttRef_11;				// Attachment for depth and stencil data. A subpass can only use a single depth (+ stencil) attachment.
	subpass_11.inputAttachmentCount;
	subpass_11.pInputAttachments;										// Attachments read from a shader.
	subpass_11.preserveAttachmentCount;
	subpass_11.pPreserveAttachments;									// Attachment not used by this subpass, but for which the data must be preserved.

	VkSubpassDependency dependency_11{};
	dependency_11.srcSubpass = VK_SUBPASS_EXTERNAL;																			// VK_SUBPASS_EXTERNAL: Refers to the implicit subpass before or after the render pass depending on whether it is specified in srcSubpass or dstSubpass.
	dependency_11.dstSubpass = 0;																								// Index of our subpass. The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL).
	dependency_11.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// Stage where to wait (for the swap chain to finish reading from the image).
	dependency_11.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// Stage where to wait. 
	dependency_11.srcAccessMask = 0;																							// Operations that wait.
	dependency_11.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;			// Operations that wait (they involve the writing of the color attachment).

	VkSubpassDescription subpass_12{};
	subpass_12.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_12.colorAttachmentCount = 1;
	subpass_12.pColorAttachments = &colorAttRef_12;
	subpass_12.pResolveAttachments = nullptr;
	subpass_12.pDepthStencilAttachment = &depthAttRef_12;
	subpass_12.inputAttachmentCount;
	subpass_12.pInputAttachments;
	subpass_12.preserveAttachmentCount;
	subpass_12.pPreserveAttachments;

	VkSubpassDependency dependency_12{};
	dependency_12.srcSubpass = VK_SUBPASS_EXTERNAL;																			// VK_SUBPASS_EXTERNAL: Refers to the implicit subpass before or after the render pass depending on whether it is specified in srcSubpass or dstSubpass.
	dependency_12.dstSubpass = 0;																								// Index of our subpass. The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL).
	dependency_12.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// Stage where to wait (for the swap chain to finish reading from the image).
	dependency_12.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// Stage where to wait. 
	dependency_12.srcAccessMask = 0;																							// Operations that wait.
	dependency_12.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;			// Operations that wait (they involve the writing of the color attachment).

	std::vector<VkAttachmentDescription> attachments_1 = { colorAtt_11, depthAtt_11, colorAtt_12, depthAtt_12 };		// Put together all the attachments that your render-pass will contain, in the same order you specified when creating the references (VkAttachmentReference).
	std::vector<VkSubpassDescription> subpasses_1 = { subpass_11, subpass_12 };
	std::vector<VkSubpassDependency> dependencies_1 = { dependency_11, dependency_12 };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments_1.size());
	renderPassInfo.pAttachments = attachments_1.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses_1.size());
	renderPassInfo.pSubpasses = subpasses_1.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies_1.size());
	renderPassInfo.pDependencies = dependencies_1.data();
	
	if (vkCreateRenderPass(c.device, &renderPassInfo, nullptr, &renderPass[0]) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");

	// Render pass 2 (Multisample Postprocessing) -------------------------

	VkSubpassDescription subpass2{};
	subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass2.colorAttachmentCount = 1;
	subpass2.pColorAttachments = &colorAttRef_2;
	subpass2.pResolveAttachments = &colorResolveAttRef;
	subpass2.pDepthStencilAttachment = nullptr;
	std::vector<VkAttachmentReference> inputAttachments = { colorInputAttRef_1, depthInputAttRef };
	subpass2.inputAttachmentCount = inputAttachments.size();
	subpass2.pInputAttachments = inputAttachments.data();	// <<< Can't input attachments read per-sample? Only per-pixel?
	subpass2.preserveAttachmentCount;
	subpass2.pPreserveAttachments;

	VkSubpassDependency dependency2{};
	dependency2.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency2.dstSubpass = 0;
	dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency2.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency2.srcAccessMask = 0;
	dependency2.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo2{};
	renderPassInfo2.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	std::vector<VkAttachmentDescription> attachments2 = { colorInputAtt_1, depthInputAtt, colorAtt_2, colorResolveAtt };
	renderPassInfo2.attachmentCount = static_cast<uint32_t>(attachments2.size());
	renderPassInfo2.pAttachments = attachments2.data();
	renderPassInfo2.subpassCount = 1;
	renderPassInfo2.pSubpasses = &subpass2;				// Array of subpasses
	renderPassInfo2.dependencyCount = 1;
	renderPassInfo2.pDependencies = &dependency2;		// Array of dependencies.
	
	if (vkCreateRenderPass(c.device, &renderPassInfo2, nullptr, &renderPass[1]) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");
}

void VulkanEnvironment::createImageResources_2x2()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;
	samplerInfo.mipLodBias = 0.0f;

	// Color_1 -------------------------------------

	createImage(
		swapChain.extent.width,
		swapChain.extent.height,
		1,
		c.msaaSamples,
		swapChain.imageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		color_1.image,
		color_1.memory);

	color_1.view = createImageView(color_1.image, swapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	// Create sampler
	if (vkCreateSampler(c.device, &samplerInfo, nullptr, &color_1.sampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create resolve color sampler!");


	// Depth -------------------------------------

	createImage(swapChain.extent.width,
		swapChain.extent.height,
		1,
		c.msaaSamples,
		findDepthFormat(),
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depth.image,
		depth.memory);

	depth.view = createImageView(depth.image, findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	// Explicitly transition the layout of the image to a depth attachment (there is no need of doing this because we take care of this in the render pass, but this is here for completeness).
	transitionImageLayout(depth.image, findDepthFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

	// Create sampler
	if (vkCreateSampler(c.device, &samplerInfo, nullptr, &depth.sampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create depth sampler!");


	// Color_2 -------------------------------------

	createImage(
		swapChain.extent.width,
		swapChain.extent.height,
		1,
		c.msaaSamples,
		swapChain.imageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,				// DEVICE_LOCAL: Memory will be used exclusively by the device, rather than being mappable to host memory. Better performance.
		color_2.image,
		color_2.memory);

	color_2.view = createImageView(color_2.image, swapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

// (14)
/// Create the swap chain framebuffers (by attaching to each of them the MSAA image, depth image, and swap chain image).
void VulkanEnvironment::createFramebuffers_2x2()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	framebuffers.resize(swapChain.views.size());
	std::vector<VkImageView> attachments;

	// Framebuffers (2) for each swapChainImage
	for (size_t i = 0; i < swapChain.views.size(); i++)
	{
		// Framebuffers 1 (multisampled basic color)
		attachments = std::vector<VkImageView>{ color_1.view, depth.view };		// Color attachment differs for every swap chain image, but the same depth image can be used by all of them because only a single subpass is running at the same time due to our semaphores.

		VkFramebufferCreateInfo framebufferInfo_1{};
		framebufferInfo_1.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo_1.renderPass = renderPass[0];							// A framebuffer can only be used with the render passes that it is compatible with, which roughly means that they use the same number and type of attachments.
		framebufferInfo_1.attachmentCount = attachments.size();
		framebufferInfo_1.pAttachments = attachments.data();					// Objects that should be bound to the respective attachment descriptions in the render pass pAttachment array.
		framebufferInfo_1.width = swapChain.extent.width;
		framebufferInfo_1.height = swapChain.extent.height;
		framebufferInfo_1.layers = 1;											// Number of layers in image arrays. If your swap chain images are single images, then layers = 1.

		if (vkCreateFramebuffer(c.device, &framebufferInfo_1, nullptr, &framebuffers[i][0]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer 1!");

		// Framebuffers 2 (multisampled postprocessing)
		attachments = std::vector<VkImageView>{ color_1.view, depth.view, color_2.view, swapChain.views[i] };

		VkFramebufferCreateInfo framebufferInfo_2{};
		framebufferInfo_2.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo_2.renderPass = renderPass[1];
		framebufferInfo_2.attachmentCount = attachments.size();
		framebufferInfo_2.pAttachments = attachments.data();
		framebufferInfo_2.width = swapChain.extent.width;
		framebufferInfo_2.height = swapChain.extent.height;
		framebufferInfo_2.layers = 1;

		if (vkCreateFramebuffer(c.device, &framebufferInfo_2, nullptr, &framebuffers[i][1]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer 2!");
	}
}

// (8)
/*
	@brief Tells Vulkan the framebuffer attachments that will be used while rendering (color, depth, multisampled images). A render-pass denotes more explicitly how your rendering happens.

	Specify subpasses and their attachments.
		- Subpasses: A single render pass can consist of multiple subpasses, which are subsequent rendering operations that depend on the contents of framebuffers in previous passes (example: a sequence of post-processing effects applied one after another). Grouping them into one render pass may give better performance.
		- Attachment references: Every subpass references one or more of the attachments that we've described.
*/
void VulkanEnvironment::createRenderPass_MS_PP()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	/*
		Process for MultiSampling + PostProcessing:

		Render pass 1:
			Subpass A attachments:
				shader -> colorResolveAttachment: Final color (many samplers)
				shader -> depthAttachment[n]: Depth (many samplers)
				X - msaaColorAttachment[n]: Color attachment for multisampling (not used: only 1 sample)
		Render pass 2:
			Subpass B attachments:
				shader -> colorResolveAttachment: Final color (many samples)
					colorResolveAttachment: Final color from RP 1 (input attachments)
					depthAttachment: Depth from RP 1 (input attachments)
				msaa -> colorAttachmentPP: Final color (swapChainImageView)
	*/

	// Attachments -------------------------

	VkAttachmentDescription colorAttachment_1{};	// SP1, 
	VkAttachmentReference colorAttachmentRef_1{};
	VkAttachmentDescription depthAttachment{};
	VkAttachmentReference depthAttachmentRef{};

	VkAttachmentDescription colorInputAtt_1;		// SP2
	VkAttachmentReference colorInputAttRef_1;
	VkAttachmentDescription depthInputAtt;
	VkAttachmentReference depthInputAttRef;

	VkAttachmentDescription colorAttachment_2{};
	VkAttachmentReference colorAttachmentRef_2{};
	VkAttachmentDescription colorResolveAttachment{};
	VkAttachmentReference colorResolveAttachmentRef{};

	// Color attachment 1 (basic multisampling rendering) (RP 1)
	colorAttachment_1.format = swapChain.imageFormat;
	colorAttachment_1.samples = c.msaaSamples;									// Single color buffer attachment, or many (multisampling).
	colorAttachment_1.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;						// What to do with the data (color and depth) in the attachment before rendering: VK_ATTACHMENT_LOAD_OP_ ... LOAD (preserve existing contents of the attachment), CLEAR (clear values to a constant at the start of a new frame), DONT_CARE (existing contents are undefined).
	colorAttachment_1.storeOp = VK_ATTACHMENT_STORE_OP_STORE;					// What to do with the data (color and depth) in the attachment after rendering:  VK_ATTACHMENT_STORE_OP_ ... STORE (rendered contents will be stored in memory and can be read later), DON_CARE (contents of the framebuffer will be undefined after rendering).
	colorAttachment_1.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;			// What to do with the stencil data in the attachment before rendering.
	colorAttachment_1.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;		// What to do with the stencil data in the attachment after rendering.
	colorAttachment_1.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;				// Layout before the render pass. Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however the layout of the pixels in memory need to be transitioned to specific layouts suitable for the operation that they're going to be involved in next (read more below).
	colorAttachment_1.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// Layout to automatically transition after the render pass finishes. VK_IMAGE_LAYOUT_ ... UNDEFINED (we don't care what previous layout the image was in, and the contents of the image are not guaranteed to be preserved), COLOR_ATTACHMENT_OPTIMAL (images used as color attachment), PRESENT_SRC_KHR (images to be presented in the swap chain), TRANSFER_DST_OPTIMAL (Images to be used as destination for a memory copy operation).

	colorAttachmentRef_1.attachment = 0;										// Specify which attachment to reference by its index in the attachment descriptions array.
	colorAttachmentRef_1.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;		// Specify the layout we would like the attachment to have during a subpass that uses this reference. The layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL will give us the best performance.

	// Depth attachment (RP 1)
	depthAttachment.format = findDepthFormat();									// Should be same format as the depth image
	depthAttachment.samples = c.msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// VK_ATTACHMENT_STORE_OP_DONT_CARE: Here, we don't care because it will not be used after drawing has finished
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about previous depth contents
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Input attachment (color) (from previous render pass)
	colorInputAtt_1 = colorAttachment_1;
	colorInputAtt_1.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorInputAtt_1.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	colorInputAttRef_1 = colorAttachmentRef_1;
	colorInputAttRef_1.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorInputAttRef_1.attachment = 0;

	// Input attachment (depth) (from previous render pass)
	depthInputAtt = depthAttachment;
	depthInputAtt.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthInputAtt.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	depthInputAttRef = depthAttachmentRef;
	depthInputAttRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthInputAttRef.attachment = 1;

	// Color attachment 2 (Multisample postprocessing) (RP 2)
	colorAttachment_2.format = swapChain.imageFormat;
	colorAttachment_2.samples = c.msaaSamples;
	colorAttachment_2.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;// VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment_2.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment_2.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment_2.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment_2.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment_2.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	colorAttachmentRef_2.attachment = 2;									// Specify which attachment to reference by its index in the attachment descriptions array.
	colorAttachmentRef_2.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// Specify the layout we would like the attachment to have during a subpass that uses this reference. The layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL will give us the best performance.

	// Final resolved color attachment (RP 2)
	colorResolveAttachment.format = swapChain.imageFormat;
	colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	colorResolveAttachmentRef.attachment = 3;
	colorResolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Render pass 1 (basic multisample rendering) -------------------------
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// VK_PIPELINE_BIND_POINT_GRAPHICS: This is a graphics subpass
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef_1;				// Attachment for color. The index of the attachment in this array is directly referenced from the fragment shader with the directive "layout(location = 0) out vec4 outColor".
	subpass.pResolveAttachments = nullptr;							// Attachments used for resolving multisampling color attachments.
	subpass.pDepthStencilAttachment = &depthAttachmentRef;			// Attachment for depth and stencil data. A subpass can only use a single depth (+ stencil) attachment.
	subpass.inputAttachmentCount;
	subpass.pInputAttachments;										// Attachments read from a shader.
	subpass.preserveAttachmentCount;
	subpass.pPreserveAttachments;									// Attachment not used by this subpass, but for which the data must be preserved.

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;																			// VK_SUBPASS_EXTERNAL: Refers to the implicit subpass before or after the render pass depending on whether it is specified in srcSubpass or dstSubpass.
	dependency.dstSubpass = 0;																							// Index of our subpass. The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL).
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// Stage where to wait (for the swap chain to finish reading from the image).
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// Stage where to wait. 
	dependency.srcAccessMask = 0;																							// Operations that wait.
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;			// Operations that wait (they involve the writing of the color attachment).

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	std::vector<VkAttachmentDescription> attachments = { colorAttachment_1, depthAttachment };		// Put together all the attachments that your render-pass will contain, in the same order you specified when creating the references (VkAttachmentReference).
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;				// Array of subpasses
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;			// Array of dependencies.

	if (vkCreateRenderPass(c.device, &renderPassInfo, nullptr, &renderPass[0]) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");

	// Render pass 2 (Multisample Postprocessing) -------------------------

	VkSubpassDescription subpass2{};
	subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass2.colorAttachmentCount = 1;
	subpass2.pColorAttachments = &colorAttachmentRef_2;
	subpass2.pResolveAttachments = &colorResolveAttachmentRef;
	subpass2.pDepthStencilAttachment = nullptr;
	std::vector<VkAttachmentReference> inputAttachments = { colorInputAttRef_1, depthInputAttRef };
	subpass2.inputAttachmentCount = inputAttachments.size();
	subpass2.pInputAttachments = inputAttachments.data();	// <<< Can't input attachments read per-sample? Only per-pixel?
	subpass2.preserveAttachmentCount;
	subpass2.pPreserveAttachments;

	VkSubpassDependency dependency2{};
	dependency2.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency2.dstSubpass = 0;
	dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency2.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency2.srcAccessMask = 0;
	dependency2.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo2{};
	renderPassInfo2.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	std::vector<VkAttachmentDescription> attachments2 = { colorInputAtt_1, depthInputAtt, colorAttachment_2, colorResolveAttachment };
	renderPassInfo2.attachmentCount = static_cast<uint32_t>(attachments2.size());
	renderPassInfo2.pAttachments = attachments2.data();
	renderPassInfo2.subpassCount = 1;
	renderPassInfo2.pSubpasses = &subpass2;				// Array of subpasses
	renderPassInfo2.dependencyCount = 1;
	renderPassInfo2.pDependencies = &dependency2;		// Array of dependencies.

	if (vkCreateRenderPass(c.device, &renderPassInfo2, nullptr, &renderPass[1]) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");
}

void VulkanEnvironment::createImageResources_MS_PP()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	VkFormat colorFormat = swapChain.imageFormat;
	VkFormat depthFormat = findDepthFormat();

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;
	samplerInfo.mipLodBias = 0.0f;

	// Color_1 -------------------------------------

	createImage(
		swapChain.extent.width,
		swapChain.extent.height,
		1,
		c.msaaSamples,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		color_1.image,
		color_1.memory);

	color_1.view = createImageView(color_1.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	// Create sampler
	if (vkCreateSampler(c.device, &samplerInfo, nullptr, &color_1.sampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create resolve color sampler!");


	// Depth -------------------------------------

	createImage(swapChain.extent.width,
		swapChain.extent.height,
		1,
		c.msaaSamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depth.image,
		depth.memory);

	depth.view = createImageView(depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	// Explicitly transition the layout of the image to a depth attachment (there is no need of doing this because we take care of this in the render pass, but this is here for completeness).
	transitionImageLayout(depth.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

	// Create sampler
	if (vkCreateSampler(c.device, &samplerInfo, nullptr, &depth.sampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create depth sampler!");


	// Color_2 -------------------------------------

	createImage(
		swapChain.extent.width,
		swapChain.extent.height,
		1,
		c.msaaSamples,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,				// DEVICE_LOCAL: Memory will be used exclusively by the device, rather than being mappable to host memory. Better performance.
		color_2.image,
		color_2.memory);

	color_2.view = createImageView(color_2.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

// (14)
/// Create the swap chain framebuffers (by attaching to each of them the MSAA image, depth image, and swap chain image).
void VulkanEnvironment::createFramebuffers_MS_PP()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	framebuffers.resize(swapChain.views.size());
	std::vector<VkImageView> attachments;

	// Framebuffers (2) for each swapChainImage
	for (size_t i = 0; i < swapChain.views.size(); i++)
	{
		// Framebuffers 1 (multisampled basic color)
		attachments = std::vector<VkImageView>{ color_1.view, depth.view };		// Color attachment differs for every swap chain image, but the same depth image can be used by all of them because only a single subpass is running at the same time due to our semaphores.

		VkFramebufferCreateInfo framebufferInfo_1{};
		framebufferInfo_1.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo_1.renderPass = renderPass[0];							// A framebuffer can only be used with the render passes that it is compatible with, which roughly means that they use the same number and type of attachments.
		framebufferInfo_1.attachmentCount = attachments.size();
		framebufferInfo_1.pAttachments = attachments.data();					// Objects that should be bound to the respective attachment descriptions in the render pass pAttachment array.
		framebufferInfo_1.width = swapChain.extent.width;
		framebufferInfo_1.height = swapChain.extent.height;
		framebufferInfo_1.layers = 1;											// Number of layers in image arrays. If your swap chain images are single images, then layers = 1.

		if (vkCreateFramebuffer(c.device, &framebufferInfo_1, nullptr, &framebuffers[i][0]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer 1!");

		// Framebuffers 2 (multisampled postprocessing)
		attachments = std::vector<VkImageView>{ color_1.view, depth.view, color_2.view, swapChain.views[i] };

		VkFramebufferCreateInfo framebufferInfo_2{};
		framebufferInfo_2.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo_2.renderPass = renderPass[1];
		framebufferInfo_2.attachmentCount = attachments.size();
		framebufferInfo_2.pAttachments = attachments.data();
		framebufferInfo_2.width = swapChain.extent.width;
		framebufferInfo_2.height = swapChain.extent.height;
		framebufferInfo_2.layers = 1;

		if (vkCreateFramebuffer(c.device, &framebufferInfo_2, nullptr, &framebuffers[i][1]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer 2!");
	}
}

void VulkanEnvironment::createRenderPass_PP()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	/*
		Process for SingleSample + PostProcessing:

		Render pass 1:
			Subpass A attachments:
				shader -> colorResolveAttachment: Final color (sampler)
				shader -> depthAttachment[n]: Depth (sampler)
				X - msaaColorAttachment[n]: Color attachment for multisampling (not used: only 1 sample)
		Render pass 2:
			Subpass B attachments:
				shader -> colorAttachmentPP: Final color (swapChainImageView)
					attachment - colorResolveAttachment: Final color from RP 1 (input attachment)
					attachment - depthAttachment: Depth from RP 1 (input attachment)
	*/

	// Attachments: -------------------------

	// First color attachment (basic rendering) (RP 1)
	VkAttachmentDescription colorAttachment_1{};
	colorAttachment_1.format = swapChain.imageFormat;
	colorAttachment_1.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment_1.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	// (msaaSamples > 1) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR;	// To do before rendering begins
	colorAttachment_1.storeOp = VK_ATTACHMENT_STORE_OP_STORE;														// To do after rendering is complete
	colorAttachment_1.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment_1.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment_1.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;													// Attachment's layout state when render pass begins
	colorAttachment_1.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;										// Attachment's layout state it has to transition to when render pass is complete

	VkAttachmentReference colorAttachmentRef_1{};
	colorAttachmentRef_1.attachment = 0;
	colorAttachmentRef_1.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;										// Attachment layout state when the render pass begins.

	// Depth attachment (RP 1)
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();							// Should be same format as the depth image
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;			// VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// VK_ATTACHMENT_STORE_OP_DONT_CARE: Here, we don't care because it will not be used after drawing has finished
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// We don't care about previous depth contents
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Input attachment (color) (from previous render pass) (RP 2)
	VkAttachmentDescription colorInputAtt_1 = colorAttachment_1;
	colorInputAtt_1.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorInputAtt_1.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorInputAttRef_1 = colorAttachmentRef_1;
	colorInputAttRef_1.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorInputAttRef_1.attachment = 0;

	// Input attachment (depth) (from previous render pass) (RP 2)
	VkAttachmentDescription depthInputAtt = depthAttachment;
	depthInputAtt.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthInputAtt.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

	VkAttachmentReference depthInputAttRef = depthAttachmentRef;
	depthInputAttRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	depthInputAttRef.attachment = 1;

	// Final color (Post-Processing) (RP 2)
	VkAttachmentDescription colorAttachment_2{};
	colorAttachment_2.format = swapChain.imageFormat;
	colorAttachment_2.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment_2.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment_2.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment_2.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment_2.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment_2.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment_2.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef_2{};
	colorAttachmentRef_2.attachment = 2;
	colorAttachmentRef_2.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Render pass 1 (basic rendering) -------------------------

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// VK_PIPELINE_BIND_POINT_GRAPHICS: This is a graphics subpass
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef_1;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;			// Attachment for depth and stencil data. A subpass can only use a single depth (+ stencil) attachment.
	subpass.inputAttachmentCount;
	subpass.pInputAttachments;										// Attachments read from a shader.
	subpass.preserveAttachmentCount;
	subpass.pPreserveAttachments;									// Attachment not used by this subpass, but for which the data must be preserved.

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;																			// VK_SUBPASS_EXTERNAL: Refers to the implicit subpass before or after the render pass depending on whether it is specified in srcSubpass or dstSubpass.
	dependency.dstSubpass = 0;																							// Index of our subpass. The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL).
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// Stage where to wait (for the swap chain to finish reading from the image).
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// Stage where to wait. 
	dependency.srcAccessMask = 0;																							// Operations that wait.
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;			// Operations that wait (they involve the writing of the color attachment).

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	std::vector<VkAttachmentDescription> attachments = { colorAttachment_1, depthAttachment };		// Put together all the attachments that your render-pass will contain, in the same order you specified when creating the references (VkAttachmentReference).
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;				// Array of subpasses
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;			// Array of dependencies.

	if (vkCreateRenderPass(c.device, &renderPassInfo, nullptr, &renderPass[0]) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");

	// Render pass 2 (Post processing) -------------------------

	VkSubpassDescription subpass2{};
	subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass2.colorAttachmentCount = 1;
	subpass2.pColorAttachments = &colorAttachmentRef_2;
	subpass2.pDepthStencilAttachment = nullptr;
	subpass2.pResolveAttachments = nullptr;
	std::vector<VkAttachmentReference> inputAttachments = { colorInputAttRef_1, depthInputAttRef };
	subpass2.inputAttachmentCount = inputAttachments.size();
	subpass2.pInputAttachments = inputAttachments.data();	// <<< Can't input attachments read per-sample? Only per-pixel?
	subpass2.preserveAttachmentCount;
	subpass2.pPreserveAttachments;

	VkSubpassDependency dependency2{};
	dependency2.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency2.dstSubpass = 0;
	dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency2.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency2.srcAccessMask = 0;
	dependency2.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo2{};
	renderPassInfo2.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	std::vector<VkAttachmentDescription> attachments2 = { colorInputAtt_1, depthInputAtt, colorAttachment_2 };
	renderPassInfo2.attachmentCount = static_cast<uint32_t>(attachments2.size());
	renderPassInfo2.pAttachments = attachments2.data();
	renderPassInfo2.subpassCount = 1;
	renderPassInfo2.pSubpasses = &subpass2;				// Array of subpasses
	renderPassInfo2.dependencyCount = 1;
	renderPassInfo2.pDependencies = &dependency2;		// Array of dependencies.

	if (vkCreateRenderPass(c.device, &renderPassInfo2, nullptr, &renderPass[1]) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass!");
}

void VulkanEnvironment::createImageResources_PP()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	VkFormat colorFormat = swapChain.imageFormat;
	VkFormat depthFormat = findDepthFormat();

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;
	samplerInfo.mipLodBias = 0.0f;

	// Color_1 -------------------------------------

	createImage(
		swapChain.extent.width,
		swapChain.extent.height,
		1,
		VK_SAMPLE_COUNT_1_BIT,	//msaaSamples,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		color_1.image,
		color_1.memory);

	color_1.view = createImageView(color_1.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	if (vkCreateSampler(c.device, &samplerInfo, nullptr, &color_1.sampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create resolve color sampler!");

	// Depth -------------------------------------

	createImage(swapChain.extent.width,
		swapChain.extent.height,
		1,
		c.msaaSamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depth.image,
		depth.memory);

	depth.view = createImageView(depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	// Explicitly transition the layout of the image to a depth attachment (there is no need of doing this because we take care of this in the render pass, but this is here for completeness).
	transitionImageLayout(depth.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

	if (vkCreateSampler(c.device, &samplerInfo, nullptr, &depth.sampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create depth sampler!");
}

// (14)
/// Create the swap chain framebuffers (by attaching to each of them the MSAA image, depth image, and swap chain image).
void VulkanEnvironment::createFramebuffers_PP()
{
	#ifdef DEBUG_ENV_CORE
		std::cout << "   " << typeid(*this).name() << "::" << __func__ << std::endl;
	#endif

	framebuffers.resize(swapChain.views.size());
	std::vector<VkImageView> attachments;

	// Framebuffers (2) for each swapChainImage
	for (size_t i = 0; i < swapChain.views.size(); i++)
	{
		// Framebuffer 1 (basic color)
		VkFramebufferCreateInfo framebufferInfo_1{};
		framebufferInfo_1.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo_1.renderPass = renderPass[0];								// A framebuffer can only be used with the render passes that it is compatible with, which roughly means that they use the same number and type of attachments.
		attachments = std::vector<VkImageView>{ color_1.view, depth.view };
		framebufferInfo_1.attachmentCount = attachments.size();
		framebufferInfo_1.pAttachments = attachments.data();						// Objects that should be bound to the respective attachment descriptions in the render pass pAttachment array.
		framebufferInfo_1.width = swapChain.extent.width;
		framebufferInfo_1.height = swapChain.extent.height;
		framebufferInfo_1.layers = 1;												// Number of layers in image arrays. If your swap chain images are single images, then layers = 1.

		if (vkCreateFramebuffer(c.device, &framebufferInfo_1, nullptr, &framebuffers[i][0]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer 1!");

		// Framebuffer 2 (post-processing)
		VkFramebufferCreateInfo framebufferInfo_2{};
		framebufferInfo_2.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo_2.renderPass = renderPass[1];
		attachments = std::vector<VkImageView>{ color_1.view, depth.view, swapChain.views[i] };
		framebufferInfo_2.attachmentCount = attachments.size();
		framebufferInfo_2.pAttachments = attachments.data();
		framebufferInfo_2.width = swapChain.extent.width;
		framebufferInfo_2.height = swapChain.extent.height;
		framebufferInfo_2.layers = 1;

		if (vkCreateFramebuffer(c.device, &framebufferInfo_2, nullptr, &framebuffers[i][1]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create framebuffer 2!");
	}
}


// Independent methods ----------------------------------------------

VkDeviceSize VulkanCore::getMinUniformBufferOffsetAlignment()
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	return deviceProperties.limits.minUniformBufferOffsetAlignment;
}

VkBool32 VulkanCore::supportsAnisotropicFiltering()
{
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	return deviceFeatures.samplerAnisotropy;
}

VkBool32 VulkanCore::largePointsSupported()
{
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	return deviceFeatures.largePoints;
}

VkBool32 VulkanCore::wideLinesSupported()
{
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	return deviceFeatures.wideLines;
}
