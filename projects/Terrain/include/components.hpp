#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <iostream>

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE		// GLM uses OpenGL depth range [-1.0, 1.0]. This macro forces GLM to use Vulkan range [0.0, 1.0].
//#define GLM_ENABLE_EXPERIMENTAL			// Required for using std::hash functions for the GLM types (since gtx folder contains experimental extensions)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>     // Generate transformations matrices with glm::rotate (model), glm::lookAt (view), glm::perspective (projection).
//#include <glm/gtx/hash.hpp>
#include "glm/gtc/type_ptr.hpp"

//#define GLFW_INCLUDE_VULKAN		// Makes GLFW load the Vulkan header with it
#include "GLFW/glfw3.h"

#include "renderer.hpp"
#include "toolkit.hpp"
#include "ECSarch.hpp"


// Singletons --------------------------------------

struct c_Engine : public Component
{
	c_Engine(Renderer& renderer);
	~c_Engine() { };
	void printInfo() const;

	Renderer& r;
	GLFWwindow* window;

	uint32_t width;		//!< pixels width  (1920/2)
	uint32_t height;	//!< pixels height (1080/2)
	float aspectRatio() const { return (float)width / height; };
};

struct c_Input : public Component
{
	c_Input() : Component("input") { };
	~c_Input() { };
	void printInfo() const;

	//GLFWwindow* window;

	// Keyboard
	bool W_press = false;
	bool S_press = false;
	bool A_press = false;
	bool D_press = false;
	bool Q_press = false;
	bool E_press = false;
	bool up_press = false;
	bool down_press = false;
	bool left_press = false;
	bool right_press = false;

	// Mouse
	bool LMB_pressed = false;
	bool RMB_pressed = false;
	bool MMB_pressed = false;
	bool LMB_justPressed = false;		//!< First frame with left mouse button pressed.
	bool RMB_justPressed = false;
	bool MMB_justPressed = false;
	double yScrollOffset = 0;
	double xpos = 0, ypos = 0;			//!< Cursor position
	double lastX = 0, lastY = 0;		//!< Mouse positions in the previous frame.
	double xOffset() const { return xpos - lastX; };
	double yOffset() const { return ypos - lastY; };
	
	// Speed
	float keysSpeed   = 50.f;	///< camera speed
	float mouseSpeed  = 0.001f;	///< Mouse sensitivity
	float scrollSpeed = 0.1f;	///< Scroll speed
};

struct c_Camera : public Component
{
	c_Camera() : Component("camera") { };
	~c_Camera() { };
	void printInfo() const;

	glm::vec3 camPos  = { 1450.f, 1450.f, 0.f };
	glm::vec3 front   = { 1,0,0 };		//!< cam front vector
	glm::vec3 right   = { 0,-1,0 };		//!< cam right vector
	glm::vec3 camUp   = { 0,0,1 };		//!< cam up vector (used in lookAt()) (computed from cross(right, front))
	glm::vec3 worldUp = { 0,0,1 };		//!< World up vector (used for elevating/descending to/from the sky)

	float fov = 1.f, minFov = 0.1f, maxFov = 2.f;	//!< FOV (rad)
	float nearViewPlane = 0.2f;						//!< Near view plane
	float farViewPlane = 4000.f;					//!< Near view plane

	glm::mat4 view;		//!< Model transformation matrix
	glm::mat4 proj;		//!< Model transformation matrix
};


// Non-Singletons --------------------------------------

struct c_Model : public Component
{
	c_Model(modelIter model) : Component("model"), model(model) { };
	~c_Model() { };
	void printInfo() const { };

	modelIter model;
};

struct c_Position : public Component
{
	c_Position(bool followCam = false, glm::vec3 position = { 0, 0, 0 }) 
		: Component("position"), pos(position), followCam(followCam) { };
	~c_Position() { };
	void printInfo() const;

	glm::vec3 pos;
	bool followCam;
};

/*
struct c_Velocity : public Component
{
	c_Velocity(glm::vec3 velocity = { 0, 0, 0 }) : Component("vel"), v(velocity) { };
	~c_Velocity() { };

	glm::vec3 v;															//!< Velocity: Moving time rate & direction
	float speed() { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); };	//!< Speed: Moving time rate 
};

struct c_Scale : public Component
{
	c_Scale(glm::vec3 scale = { 1, 1, 1 }) : Component("scale"), scale(scale) { };
	~c_Scale() { };

	glm::vec3 scale;
};
*/

struct c_ModelMatrix : public Component
{
	c_ModelMatrix(float scaleFactor = 1);
	~c_ModelMatrix() { };
	void printInfo() const;

	glm::vec3 scaling;
	glm::vec4 rotQuat     = { 1, 0, 0, 0 };
	glm::vec3 translation = { 0, 0, 0 };
	glm::mat4 modelMatrix;			//!< Model transformation matrix
};

#endif
