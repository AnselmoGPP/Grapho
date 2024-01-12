#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <vector>
#include <optional>					// std::optional<uint32_t> (Wrapper that contains no value until you assign something to it. Contains member has_value())
#include <mutex>

//#include "vulkan/vulkan.h"		// From LunarG SDK. Can be used for off-screen rendering
//#define GLFW_INCLUDE_VULKAN		// Makes GLFW load the Vulkan header with it
//#include "GLFW/glfw3.h"

#include "input.hpp"

//#define DEBUG_ENV_INFO			// Basic info
//#define DEBUG_ENV_CORE			// Standards: NDEBUG, _DEBUG
#define VAL_LAYERS					// Enable Validation layers

#ifdef VAL_LAYERS
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

typedef std::vector<VkFramebuffer> framebufferSet;

class VulkanCore;
class VulkanEnvironment;

/// Structure for storing vector indices of the queue families we want. 
/** Note that graphicsFamilyand presentFamily could refer to the same queue family, but we included them separately because sometimes they are in different queue families. */
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;		///< Queue family capable of computer graphics.
	std::optional<uint32_t> presentFamily;		///< Queue family capable of presenting to our window surface.
	bool isComplete();							///< Checks whether all members have value.
};

/// Structure containing details about the swap chain that must be checked. 
/** Though a swap chain may be available, it may not be compatible with our window surface, so we need to query for some detailsand check them.This struct will contain these details. */
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR		capabilities;		// Basic surface capabilities: min/max number of images in swap chain, and min/max width/height of images.
	std::vector<VkSurfaceFormatKHR> formats;			// Surface formats: pixel format, color space.
	std::vector<VkPresentModeKHR>	presentModes;		// Available presentation modes
};

/// Image used as attachment in a render pass. One per render pass.
struct Image
{
	Image();
	void destroy(VulkanEnvironment* e);

	VkImage			image;		//!< Image object
	VkDeviceMemory	memory;		//!< Device memory object
	VkImageView		view;		//!< References a part of the image to be used (subset of its pixels). Required for being able to access it.
	VkSampler		sampler;	//!< Images are accessed through image views rather than directly
};

struct SwapChain
{
	SwapChain();
	void destroy(VkDevice device);

	VkSwapchainKHR								swapChain;		//!< Swap chain object.
	std::vector<VkImage>						images;			//!< List. Opaque handle to an image object.
	std::vector<VkImageView>					views;			//!< List. Opaque handle to an image view object. It allows to use VkImage in the render pipeline. It's a view into an image; it describes how to access the image and which part of the image to access.

	VkFormat									imageFormat;
	VkExtent2D									extent;
};

struct DeviceData
{
	void fillWithDeviceData(VkPhysicalDevice physicalDevice);
	void printData();

	VkPhysicalDeviceProperties	deviceProperties;	//!< Device properties: Name, type, supported Vulkan version...
	VkPhysicalDeviceFeatures	deviceFeatures;		//!< Device features: Texture compression, 64 bit floats, multi-viewport rendering...

	// Properties (redundant)
	uint32_t apiVersion;
	uint32_t driverVersion;
	uint32_t vendorID;
	uint32_t deviceID;
	VkPhysicalDeviceType deviceType;
	std::string deviceName;

	uint32_t maxUniformBufferRange;						//!< Max. uniform buffer object size (https://community.khronos.org/t/uniform-buffer-not-big-enough-how-to-handle/103981)
	uint32_t maxPerStageDescriptorUniformBuffers;
	uint32_t maxDescriptorSetUniformBuffers;
	uint32_t maxImageDimension2D;						//!< Useful for selecting a physical device
	uint32_t maxMemoryAllocationCount;					//!< Max. number of valid memory objects
	VkSampleCountFlags framebufferColorSampleCounts;	//!< Useful for getting max. number of MSAA
	VkSampleCountFlags framebufferDepthSampleCounts;	//!< Useful for getting max. number of MSAA
	VkDeviceSize minUniformBufferOffsetAlignment;		//!< Useful for aligning dynamic descriptor sets (usually == 32 or 256)

	// Features (redundant)
	VkBool32 samplerAnisotropy;							//!< Does physical device supports Anisotropic Filtering (AF)?
	VkBool32 largePoints;
	VkBool32 wideLines;
};


class VulkanCore
{
public:
	VulkanCore(IOmanager& io);

	const bool add_MSAA = true;			//!< Shader MSAA (MultiSample AntiAliasing). 
	const bool add_SS = true;			//!< Sample shading. This can solve some problems from shader MSAA (example: only smoothens out edges of geometry but not the interior filling) (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#primsrast-sampleshading).
	const unsigned numRenderPasses = 2;	//!< Number of render passes

	VkInstance					instance;			//!< Opaque handle to an instance object. There is no global state in Vulkan and all per-application state is stored here.
	VkDebugUtilsMessengerEXT	debugMessenger;		//!< Opaque handle to a debug messenger object (the debug callback is part of it).
	VkSurfaceKHR				surface;			//!< Opaque handle to a surface object (abstract type of surface to present rendered images to)

	VkPhysicalDevice			physicalDevice;		//!< Opaque handle to a physical device object.
	VkSampleCountFlagBits		msaaSamples;		//!< Number of samples used for MSAA (MultiSampling AntiAliasing)
	VkDevice					device;				//!< Opaque handle to a device object.
	DeviceData					deviceData;			//!< Physical device properties and features.

	VkQueue						graphicsQueue;		//!< Opaque handle to a queue object (computer graphics).
	VkQueue						presentQueue;		//!< Opaque handle to a queue object (presentation to window surface).

	int memAllocObjects;							//!< Number of memory allocated objects (must be <= maxMemoryAllocationCount). Incremented each vkAllocateMemory call; decremented each vkFreeMemory call.

	SwapChainSupportDetails	querySwapChainSupport();
	QueueFamilyIndices findQueueFamilies();
	void destroy();

private:
	IOmanager& io;

	const std::vector<const char*> requiredValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };	//!< Swap chain: Queue of images that are waiting to be presented to the screen. Our application will acquire such an image to draw to it, and then return it to the queue. Its general purpose is to synchronize the presentation of images with the refresh rate of the screen.

	void initWindow();
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();

	bool checkValidationLayerSupport(const std::vector<const char*>& requiredLayers);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	std::vector<const char*> getRequiredExtensions();
	bool checkExtensionSupport(const char* const* requiredExtensions, uint32_t reqExtCount);
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	int evaluateDevice(VkPhysicalDevice device);
	VkSampleCountFlagBits getMaxUsableSampleCount(bool getMinimum);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails	querySwapChainSupport(VkPhysicalDevice device);
};


class VulkanEnvironment
{
	IOmanager& io;

public:
	VulkanEnvironment(IOmanager& io);
	~VulkanEnvironment();

	VulkanCore c;

	void			createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	uint32_t		findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void			transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	VkCommandBuffer	beginSingleTimeCommands();
	void			endSingleTimeCommands(VkCommandBuffer commandBuffer);
	VkImageView		createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	VkFormat		findDepthFormat();
	//IOmanager*		getWindowManager();

	void			recreate_Images_RenderPass_SwapChain();
	void			cleanup_Images_RenderPass_SwapChain();
	void			cleanup();

	// Main member variables:
	VkCommandPool commandPool;				//!< Opaque handle to a command pool object. It manages the memory that is used to store the buffers, and command buffers are allocated from them. 

	VkRenderPass renderPass[2];				//!< Opaque handle to a render pass object. Describes the attachments to a swapChainFramebuffer.
	Image color_1;							// Basic color (one or more samples)
	Image depth;							// Depth buffer (one or more samples)
	Image color_2;							// For postprocessing multiple samples (if used)
	SwapChain swapChain;					// Final color. Swapchain elements.

	std::vector<std::array<VkFramebuffer, 2>> framebuffers;	//!< List. Opaque handle to a framebuffer object (set of attachments, including the final image to render). Access: swapChainFramebuffers[numSwapChainImages][attachment]. First attachment: main color. Second attachment: post-processing

	const uint32_t inputAttachmentCount;

	std::mutex queueMutex;					//!< Controls that vkQueueSubmit is not used in two threads simultaneously (Environment -> endSingleTimeCommands(), and Renderer -> createCommandBuffers)
	std::mutex mutCommandPool;				//!< Command pool cannot be used simultaneously in 2 different threads. Problem: It is used at command buffer creation (Renderer, 1st thread, at updateCB), and beginSingleTimeCommands and endSingleTimeCommands (Environment, 2nd thread, indirectly used in loadAndCreateTexture & fullConstruction), and indirectly sometimes (command buffer).

private:
	void createSwapChain();
	void createSwapChainImageViews();

	void createCommandPool();

	void createRenderPass();
	void createImageResources();
	void createFramebuffers();

	// Helper methods:
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool hasStencilComponent(VkFormat format);

	void createRenderPass_PP();
	void createImageResources_PP();
	void createFramebuffers_PP();

	void createRenderPass_MS_PP();
	void createImageResources_MS_PP();
	void createFramebuffers_MS_PP();

	void createRenderPass_2x2();
	void createImageResources_2x2();
	void createFramebuffers_2x2();
};

#endif