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
	struct c_Cam_Sphere;
	struct c_Cam_Plane_polar;
	struct c_Cam_Plane_free;
	struct c_Cam_FPV;
struct c_Lights;
struct c_Sky;
struct c_Model;
	struct c_Model_normal;
	struct c_Model_planet;
	struct c_Model_grassPlanet;
struct c_Move;
struct c_ModelMatrix;


// enumerations --------------------------------------

enum MoveType { followCam, followCamXY, skyOrbit, sunOrbit };
enum class UboType { noData, mvp, planet, grassPlanet, atmosphere, mvpncl };		//!< Tells the system how to update uniforms and what type of c_Model's child was created.


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

	int getWidth() const;
	int getHeight() const;
	float getAspectRatio() const;
};

struct c_Input : public Component
{
	c_Input() : Component(CT::input) { };
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
	static enum camMode { sphere, plane_polar, plane_polar_sphere, plane_free, fpv };

	c_Camera(camMode camMode, glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed);
	virtual ~c_Camera() { };
	void printInfo() const;

	camMode mode;									//!> Sphere, Plane_polar, Plane_free, FPV

	// Position & orientation
	glm::vec3 camPos;
	glm::vec3 front   = { 1, 0, 0 };				//!< cam front vector
	glm::vec3 right   = { 0,-1, 0 };				//!< cam right vector
	glm::vec3 camUp   = { 0, 0, 1 };				//!< cam up vector (used in lookAt()) (computed from cross(right, front))
	
	// View
	glm::mat4 view;									//!< Model transformation matrix
	glm::mat4 proj;									//!< Model transformation matrix
	
	float fov = 1.f;								//!< FOV (rad)
	float minFov = 0.1f;
	float maxFov = 2.f;

	float nearViewPlane = 0.2f;						//!< Near view plane
	float farViewPlane = 4000.f;					//!< Near view plane
	
	// Cam speed
	float keysSpeed;								//!< camera speed
	float mouseSpeed;								//!< Mouse sensitivity
	float scrollSpeed;								//!< Scroll speed
};

struct c_Cam_Sphere : public c_Camera
{
	c_Cam_Sphere();
	~c_Cam_Sphere() { };

	glm::vec3 worldUp;				//!< World up vector (used for elevating/descending to/from the sky)
	glm::vec3 euler;				//!< Euler angles: Pitch (x), Roll (y), Yaw (z)

	glm::vec3 center;
	float maxPitch;
	float radius;
	float minRadius;
	float maxRadius;
};

struct c_Cam_Plane_polar : public c_Camera
{
	c_Cam_Plane_polar();
	~c_Cam_Plane_polar() { };

	glm::vec3 worldUp;		//!< World up vector (used for elevating/descending to/from the sky)
	glm::vec3 euler;		//!< Euler angles: Pitch (x), Roll (y), Yaw (z)
};

struct c_Cam_Plane_polar_sphere : public c_Camera
{
	c_Cam_Plane_polar_sphere();
	~c_Cam_Plane_polar_sphere() { };

	glm::vec3 worldUp;				//!< World up vector (used for elevating/descending to/from the sky)
	glm::vec3 euler;				//!< Euler angles: Pitch (x), Roll (y), Yaw (z)

	glm::vec3 center;
	float spinSpeed;				//!< Spinning speed
};

struct c_Cam_Plane_free : public c_Camera
{
	c_Cam_Plane_free();
	~c_Cam_Plane_free() { };

	float spinSpeed;	//!< Spinning speed
};

struct c_Cam_FPV : public c_Camera
{
	c_Cam_FPV();
	~c_Cam_FPV() { };
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

//struct c_World
//{
//	c_World() : Component(CT::world) { };
//	~c_World() { };
//	void printInfo() const { };
//
//	enum worldType { flat, sphere };
//};

// Non-Singletons --------------------------------------

struct c_Model : public Component	//!< Its children doesn't have the same interface, so UboType enum is used to identify them.
{
	c_Model(UboType ubo_type) : Component(CT::model), ubo_type(ubo_type) { };
	virtual ~c_Model() { };
	virtual void printInfo() const = 0;

	UboType ubo_type;	//!< Tells the system what type of c_Model's child was created and how to update uniforms.
};

struct c_Model_normal : public c_Model
{
	c_Model_normal(modelIter model, UboType uboType) : c_Model(uboType), model(model) { };
	~c_Model_normal() { };
	void printInfo() const override { };

	modelIter model;
};

struct c_Model_planet : public c_Model
{
	c_Model_planet(Planet* planet) : c_Model(UboType::planet), planet(planet) { }
	~c_Model_planet() { if (planet) delete planet; }
	void printInfo() const override { }

	Planet* planet;
};

struct c_Model_grassPlanet : public c_Model
{
	c_Model_grassPlanet(GrassSystem_planet* grass) : c_Model(UboType::grassPlanet), grass(grass) { };
	~c_Model_grassPlanet() { /*if (grass) delete grass;*/ };
	void printInfo() const override { };

	GrassSystem_planet* grass;
};

struct ModelParams
{
	ModelParams(glm::vec3& scale, glm::vec4& rotQuat, glm::vec3& pos) : scale(scale), rotQuat(rotQuat), pos(pos) { }

	glm::vec3 scale = { 1, 1, 1 };
	glm::vec4 rotQuat = { 1, 0, 0, 0 };
	glm::vec3 pos = { 0, 0, 0 };		// glm::vec3 translation = { 0, 0, 0 };
};

/// Determines the Model matrix parameters (scale, rotation, translation/position)
struct c_ModelParams : public Component
{
	c_ModelParams(glm::vec3 scale = {1, 1, 1}, glm::vec4 rotQuat = { 1, 0, 0, 0 }, glm::vec3 position = { 0, 0, 0 });
	~c_ModelParams() { };
	void printInfo() const;

	std::vector<ModelParams> mp;	// model parameters (scale, rotation, position)
};

/// Determines the move type
struct c_Move : public Component
{
	c_Move(MoveType moveType, float jumpStep = 0);
	~c_Move() { };
	void printInfo() const;

	MoveType moveType;
	float jumpStep = 0;			//!< Used in discrete moves, not uniform moves.
};

bool itemSupported_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);

/// Determines the distribution over a surface of one or more instances of the same model (or set of models).
struct c_Distributor : public Component
{
	c_Distributor(
		unsigned minDepth, 
		unsigned posture = 1, 
		bool(*grassSupported_callback)(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers) = itemSupported_callback, std::vector<std::shared_ptr<Noiser>> noisers = std::vector<std::shared_ptr<Noiser>>());
	~c_Distributor() { };
	void printInfo() const { };

	unsigned minDepth;
	unsigned posture;	// 1 (vertical),  2 (face cam)

	bool(*itemSupported) (const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);	//!< Evaluated each item's posible position. Callback used by the client for evaluating world-related conditions and forcing or negating item rendering.

	std::vector<std::shared_ptr<Noiser>> noisers;
};

#endif
