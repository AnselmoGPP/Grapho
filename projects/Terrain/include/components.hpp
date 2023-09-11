#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <iostream>

#include "terrain.hpp"

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE		// GLM uses OpenGL depth range [-1.0, 1.0]. This macro forces GLM to use Vulkan range [0.0, 1.0].
//#define GLM_ENABLE_EXPERIMENTAL			// Required for using std::hash functions for the GLM types (since gtx folder contains experimental extensions)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>     // Generate transformations matrices with glm::rotate (model), glm::lookAt (view), glm::perspective (projection).
//#include <glm/gtx/hash.hpp>
#include "glm/gtc/type_ptr.hpp"

//#define GLFW_INCLUDE_VULKAN				// Makes GLFW load the Vulkan header with it
#include "GLFW/glfw3.h"

#include "renderer.hpp"
#include "toolkit.hpp"
#include "ECSarch.hpp"


// Prototypes --------------------------------------

struct c_Engine;
struct c_Input;
struct c_Camera;
struct c_Lights;
struct c_Sky;

struct c_Model;
	struct c_Model_single;
	struct c_Model_planet;
struct c_Move;
struct c_ModelMatrix;
struct c_Planet;


// enumerations --------------------------------------

enum MoveType { followCam, followCamXY, skyOrbit, sunOrbit };
enum class ModelType { normal, planet };
enum class UboType { noData, mvp, planet };		//!< Determine the type of data a model requires for updating its UBO


// Singletons --------------------------------------

struct c_Engine : public Component
{
	c_Engine(Renderer& renderer);
	~c_Engine() { };
	void printInfo() const;

	Renderer& r;
	IOmanager& io;
	long double time;
	size_t frameCount;			//!< Number of current frame being created [0, SIZE_MAX). If it's 0, no frame has been created yet. If render-loop finishes, the last value is kept. For debugging purposes.

	int width() const;
	int height() const;
	float aspectRatio() const;
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
};

struct c_Camera : public Component
{
	c_Camera(unsigned mode);
	~c_Camera() { };
	void printInfo() const;

	// Position & orientation
	glm::vec3 camPos  = { 0, 0, 0 };
	glm::vec3 front   = { 1, 0, 0 };		//!< cam front vector
	glm::vec3 right   = { 0,-1, 0 };		//!< cam right vector
	glm::vec3 camUp   = { 0, 0, 1 };		//!< cam up vector (used in lookAt()) (computed from cross(right, front))
	glm::vec3 worldUp = { 0, 0, 1 };		//!< World up vector (used for elevating/descending to/from the sky)
	glm::vec3 euler   = { 0, 0, 0 };		//!< Euler angles: Pitch (x), Roll (y), Yaw (z)

	// View
	glm::mat4 view;		//!< Model transformation matrix
	glm::mat4 proj;		//!< Model transformation matrix

	float fov = 1.f, minFov = 0.1f, maxFov = 2.f;	//!< FOV (rad)
	float nearViewPlane = 0.2f;						//!< Near view plane
	float farViewPlane = 4000.f;					//!< Near view plane

	// Sphere cam
	glm::vec3 center = { 0, 0, 0 };
	float maxPitch = 0;
	float radius = 0;
	float minRadius = 0;
	float maxRadius = 0;

	// Cam speed
	float keysSpeed = 50.f;		//!< camera speed
	float spinSpeed = 0.05f;	//!< Spinning speed
	float mouseSpeed = 0.001f;	//!< Mouse sensitivity
	float scrollSpeed = 0.1f;	//!< Scroll speed
};

struct c_Lights : public Component
{
	c_Lights(unsigned count);
	~c_Lights() { };
	void printInfo() const;

	LightSet lights;
};

struct c_Sky : public Component
{
	c_Sky(float skySpeed, float skyAngle, float sunSpeed, float sunAngle, float sunDist);
	~c_Sky() { };
	void printInfo() const;
	
	const float eclipticAngle;		//!< Angle between planet's rotation axis & ecliptic plane
	const float skySpeed;			//!< Angular velocity (rad/s)
	const float sunSpeed;			//!< Angular velocity (rad/s)
	const float sunDist;			//!< Dist. to sun
	const float skyAngle_0;			//!< Initial angle
	const float sunAngle_0;			//!< Initial angle

	float skyAngle;					//!< Current_angle - initial_angle
	float sunAngle;					//!< Current_angle - initial_angle
	glm::vec3 sunDir;				//!< Direction to sun
};

// Non-Singletons --------------------------------------

struct c_Model : public Component
{
	c_Model(ModelType model_type, UboType ubo_type) 
		: Component("model"), model_type(model_type), ubo_type(ubo_type) { };
	~c_Model() { };
	virtual void printInfo() const = 0;

	ModelType model_type;
	UboType ubo_type;
};

struct c_Model_normal : public c_Model
{
	c_Model_normal(modelIter model, UboType ubo_type)
		: c_Model(ModelType::normal, ubo_type), model(model) { };
	~c_Model_normal() { };
	void printInfo() const override { };

	modelIter model;
};

struct c_Model_planet : public c_Model
{
	c_Model_planet(Planet* planet) 
		: c_Model(ModelType::planet, UboType::planet), planet(planet) { };
	~c_Model_planet() { };
	void printInfo() const override { if(planet) delete planet; };

	Planet* planet;

	//Planet(Renderer* renderer, Noiser* noiseGenerator, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency);
	//Sphere(Renderer* renderer, ______________________, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency);
};

/// Determines the scaling for the Model matrix 
struct c_Scale : public Component
{
	c_Scale() : Component("scale") { };
	c_Scale(glm::vec3 scale) : Component("scale"), scale(scale) { };
	c_Scale(float scale) : Component("scale"), scale(scale, scale, scale) { };
	~c_Scale() { };
	void printInfo() const;

	glm::vec3 scale = { 1, 1, 1 };
};

/// Determines the rotation (rotation quaternion) for the Model matrix 
struct c_Rotation : public Component
{
	c_Rotation() : Component("rotation") { };
	c_Rotation(glm::vec4 rotQuat) : Component("rotation"), rotQuat(rotQuat) { };
	~c_Rotation() { };
	void printInfo() const;

	glm::vec4 rotQuat = { 1, 0, 0, 0 };
};

/// Determines the translation for the Model matrix 
struct c_Move : public Component
{
	c_Move(MoveType moveType, float jumpStep, glm::vec3 position = { 0, 0, 0 });
	c_Move(MoveType moveType);
	~c_Move() { };
	void printInfo() const;

	MoveType moveType;

	glm::vec3 pos     = { 0, 0, 0 };		// glm::vec3 translation = { 0, 0, 0 };
	glm::vec4 rotQuat = { 1, 0, 0, 0 };
	//glm::vec3 scale   = { 1, 1, 1 };

	float jumpStep = 0;			//!< Used in discrete moves, not uniform moves.
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
	c_ModelMatrix();
	c_ModelMatrix(float scale);
	c_ModelMatrix(glm::vec4 rotQuat);
	c_ModelMatrix(float scale, glm::vec4 rotQuat);
	~c_ModelMatrix() { };
	void printInfo() const;

	glm::vec3 scale = { 1, 1, 1 };
	//glm::vec4 rotQuat     = { 1, 0, 0, 0 };
	//glm::vec3 translation = { 0, 0, 0 };

	glm::mat4 modelMatrix;			//!< Model transformation matrix
};

struct c_Planet : public Component
{
	c_Planet(Renderer* renderer, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency);
	~c_Planet() { };
	void printInfo() const;

	Planet planet;

	//Renderer* renderer;
	//LightSet& lights;
	size_t rootCellSize;
	size_t numSideVertex;
	size_t numLevels;
	size_t minLevel;
	float distMultiplier;
	float radius;
	glm::vec3 nucleus;
	bool transparency;

	glm::vec3 scale = { 1, 1, 1 };
	//glm::vec4 rotQuat     = { 1, 0, 0, 0 };
	//glm::vec3 translation = { 0, 0, 0 };

	glm::mat4 modelMatrix;			//!< Model transformation matrix
};


#endif
