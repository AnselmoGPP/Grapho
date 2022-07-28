#ifndef INPUT_HPP
#define INPUT_HPP

#include "camera.hpp"

/**
	@brief The purpose of this class is mainly to serve as windowUserPointer and to hold all the callbacks. 
	Furthermore, it is a layer between loopManager and other classes that require input (like cameras).
	windowUserPointer: Pointer accessible from callbacks. Each window has one, this can be used for any 
	purpose you need, and GLFW will not modifY it throughout the life-time of the window.
*/
class Input
{
public:
	Input(GLFWwindow* window, Camera* cam);

	GLFWwindow* window;
	Camera* cam;

	bool framebufferResized = false;	///< Many drivers/platforms trigger VK_ERROR_OUT_OF_DATE_KHR after window resize, but it's not guaranteed. This variable handles resizes explicitly.

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);		///< Callback for window resizing.
	static void mouseScroll_callback(GLFWwindow* window, double xoffset, double yoffset);	///< Callback for mouse scroll.
};

#endif