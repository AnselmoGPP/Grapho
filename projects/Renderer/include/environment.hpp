#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <vector>
#include <optional>					// std::optional<uint32_t> (Wrapper that contains no value until you assign something to it. Contains member has_value())
#include <mutex>

#include "vulkan/vulkan.h"			// From LunarG SDK. Can be used for off-screen rendering
//#define GLFW_INCLUDE_VULKAN		// Makes GLFW load the Vulkan header with it
#include "GLFW/glfw3.h"


#define DEGUB						// Standards: NDEBUG, _DEBUG
#ifdef RELEASE
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

typedef std::vector<VkFramebuffer> framebufferSet;

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

/// Structure for storing all the attachments of a set of VkFramebuffer.
/** Each framebuffer has a number of attachments. Each swapChainImage has one or more framebuffers associated (framebufferSet). Each framebufferSet have the same attachments the other sets have (i.e. they share a common group of attachments)  */
struct Framebuffer
{
	VkRenderPass	renderPass;

	std::vector<framebufferSet> swapChainFramebuffer;	///< List. Opaque handle to a framebuffer object (set of attachments, including the final image to render). Access: swapChainFramebuffers[numSwapChainImages][numRenderPasses].

	VkImage			colorImage;			///< For MSAA. One per render pass
	VkDeviceMemory	colorImageMemory;	///< For MSAA. One per render pass
	VkImageView		colorImageView;		///< For MSAA. RenderPass attachment. One per render pass

	VkImage			depthImage;			///< Depth buffer (image object). One per render pass
	VkDeviceMemory	depthImageMemory;	///< Depth buffer memory (memory object). One per render pass
	VkImageView		depthImageView;		///< Depth buffer image view (images are accessed through image views rather than directly). RenderPass attachment. One per render pass

	//std::vector<VkImageView>	swapChainImageViews;
};

// Image used as attachment in a render pass. One per render pass.
struct Image
{
	Image();
	void destroyImage(VkDevice device);

	VkImage			image;
	VkDeviceMemory	memory;
	VkImageView		view;
	VkSampler		sampler;	//!< Images are accessed through image views rather than directly
};

struct swapChain
{
	VkSwapchainKHR								swapChain;				//!< Swap chain object.
	std::vector<VkImage>						swapChainImages;		//!< List. Opaque handle to an image object.
	std::vector<VkImageView>					swapChainImageViews;	//!< List. Opaque handle to an image view object. It allows to use VkImage in the render pipeline. It's a view into an image; it describes how to access the image and which part of the image to access.
	std::vector<std::array<VkFramebuffer, 2>>	swapChainFramebuffers;	//!< List. Opaque handle to a framebuffer object (set of attachments, including the final image to render). Access: swapChainFramebuffers[numSwapChainImages][attachment]. First attachment: main color. Second attachment: post-processing

	VkFormat									swapChainImageFormat;
	VkExtent2D									swapChainExtent;
};

// Render pass abstract data type. Contains the attachments (except the swapchain images)
//struct renderPass
//{
//	std::vector<VkRenderPass> renderPass;
//
//	Image color_1;
//	Image depth;
//	Image color_2;
//
//	virtual void createRenderPass();
//	virtual void createImageResources();
//	virtual void createFramebuffers();
//};
//
//// Render pass using multisample (MS) and Postprocessing (PP)
//struct renderPass_MS_PP : renderPass
//{
//	std::vector<VkRenderPass> renderPass;
//
//	void createRenderPass() override;
//	void createImageResources() override;
//	void createFramebuffers() override;
//
//	Image color_1;
//	Image depth;
//	Image color_2;
//};

class VulkanCore
{
	const std::vector<const char*> requiredValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };	//!< Swap chain: Queue of images that are waiting to be presented to the screen. Our application will acquire such an image to draw to it, and then return it to the queue. Its general purpose is to synchronize the presentation of images with the refresh rate of the screen.

public:
	VulkanCore();

	uint32_t width = 1920 / 2;		// <<< Does this change when recreating swap chain?
	uint32_t height = 1080 / 2;

	bool printInfo = true;
	const bool add_MSAA = true;		//!< Shader MSAA (MultiSample AntiAliasing). 
	const bool add_SS = true;			//!< Sample shading. This can solve some problems from shader MSAA (example: only smoothens out edges of geometry but not the interior filling) (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#primsrast-sampleshading).
	const unsigned numRenderPasses = 2;	//!< Number of render passes

	GLFWwindow*					window;				//!< Opaque window object.
	VkInstance					instance;			//!< Opaque handle to an instance object. There is no global state in Vulkan and all per-application state is stored here.
	VkDebugUtilsMessengerEXT	debugMessenger;		//!< Opaque handle to a debug messenger object (the debug callback is part of it).
	VkSurfaceKHR				surface;			//!< Opaque handle to a surface object (abstract type of surface to present rendered images to)

	VkPhysicalDevice			physicalDevice;		//!< Opaque handle to a physical device object.
	VkSampleCountFlagBits		msaaSamples;		//!< Number of samples for MSAA (MultiSampling AntiAliasing)
	VkDevice					device;				//!< Opaque handle to a device object.

	VkQueue						graphicsQueue;		//!< Opaque handle to a queue object (computer graphics).
	VkQueue						presentQueue;		//!< Opaque handle to a queue object (presentation to window surface).

	bool supportsAF;								//!< Does physical device supports Anisotropic Filtering (AF)?
	VkDeviceSize minUniformBufferOffsetAlignment;	//!< Useful for aligning dynamic descriptor sets (usually == 32 or 256)

	SwapChainSupportDetails	querySwapChainSupport();
	QueueFamilyIndices findQueueFamilies();

private:

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
	VkDeviceSize getMinUniformBufferOffsetAlignment();
	bool supportsAnisotropicFiltering();
	VkBool32 largePointsSupported();
	VkBool32 wideLinesSupported();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails	querySwapChainSupport(VkPhysicalDevice device);
};

/**
	@class VulkanEnvironment
	@brief Stores the (global) state of a Vulkan application.

	Manages different objects:
	<ul>
		<li>Window</li>
		<li>Vulkan instance</li>
		<li>surface
		<li>physical device</li>
		<li>logical device (+ queues, swap chain)
		<li>render pass</li>
		<li>framebuffer</li>
		<li>command pool</li>
	</ul>
*/
class VulkanEnvironment
{
public:
	VulkanEnvironment();
	virtual ~VulkanEnvironment();

	VulkanCore c;

	void			createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	uint32_t		findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void			transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	VkCommandBuffer	beginSingleTimeCommands();
	void			endSingleTimeCommands(VkCommandBuffer commandBuffer);
	VkImageView		createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	void			DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void			recreate_Images_RenderPass_SwapChain();
	void			cleanup_Images_RenderPass_SwapChain();
	void			cleanup();

	// Main member variables:
	VkFormat					swapChainImageFormat;				//!< Swap chain format.
	VkExtent2D					swapChainExtent;					//!< Swap chain extent.
	
	VkCommandPool				commandPool;						//!< Opaque handle to a command pool object. It manages the memory that is used to store the buffers, and command buffers are allocated from them. 

	VkRenderPass				renderPass[2];						//!< Opaque handle to a render pass object. Describes the attachments to a swapChainFramebuffer.
	std::vector<std::array<VkFramebuffer, 2>> swapChainFramebuffers;//!< List. Opaque handle to a framebuffer object (set of attachments, including the final image to render). Access: swapChainFramebuffers[numSwapChainImages][attachment]. First attachment: main color. Second attachment: post-processing

	VkSwapchainKHR				swapChain;							//!< Swap chain object.
	std::vector<VkImage>		swapChainImages;					//!< List. Opaque handle to an image object.
	std::vector<VkImageView>	swapChainImageViews;				//!< List. Opaque handle to an image view object. It allows to use VkImage in the render pipeline. It's a view into an image; it describes how to access the image and which part of the image to access.

	Image color_1;		// Basic color (one or more samples)
	Image depth;		// Depth buffer (one or more samples)
	Image color_2;		// for postprocessing multiple samples (if used)

	const uint32_t inputAttachmentCount;
	// Additional variables

	std::mutex queueMutex;							//!< Controls that vkQueueSubmit is not used in two threads simultaneously (Environment -> endSingleTimeCommands(), and Renderer -> createCommandBuffers)
	std::mutex mutCommandPool;						//!< Command pool cannot be used simultaneously in 2 different threads. Problem: It is used at command buffer creation (Renderer, 1st thread, at updateCB), and beginSingleTimeCommands and endSingleTimeCommands (Environment, 2nd thread, indirectly used in loadAndCreateTexture & fullConstruction), and indirectly sometimes (command buffer).

private:
	void createSwapChain();
	void createSwapChainImageViews();

	void createCommandPool();

	virtual void createRenderPass() = 0;
	virtual void createImageResources() = 0;
	virtual void createFramebuffers() = 0;

	// Helper methods:
	VkSurfaceFormatKHR		chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR		chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D				chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkFormat				findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool					hasStencilComponent(VkFormat format);

protected:
	VkFormat				findDepthFormat();
};

/// Creation of render pass & framebuffer (MSAA + postprocessing)
class VulkanEnv_MS_PP : public VulkanEnvironment
{
public:
	VulkanEnv_MS_PP();
	~VulkanEnv_MS_PP();



private:
	void createRenderPass() override;
	void createImageResources() override;
	void createFramebuffers() override;
};

/// Creation of render pass & framebuffer (postprocessing)
class VulkanEnv_PP : public VulkanEnvironment
{
public:
	VulkanEnv_PP();
	~VulkanEnv_PP();

private:
	void createRenderPass() override;
	void createImageResources() override;
	void createFramebuffers() override;
};


#endif