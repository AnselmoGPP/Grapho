#ifndef CAMERA_HPP
#define CAMERA_HPP

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE		// GLM uses OpenGL depth range [-1.0, 1.0]. This macro forces GLM to use Vulkan range [0.0, 1.0].
//#define GLM_ENABLE_EXPERIMENTAL			// Required for using std::hash functions for the GLM types (since gtx folder contains experimental extensions)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>     // Generate transformations matrices with glm::rotate (model), glm::lookAt (view), glm::perspective (projection).
//#include <glm/gtx/hash.hpp>
#include "glm/gtc/type_ptr.hpp"
#include "GLFW/glfw3.h"


/**
* @brief ADT. Processes input and calculates the view and projection matrices.
*
* Given some input (keyboard, mouse movement, mouse scroll), processes it (gets Euler angles, vectors and matrices) and gets the view and projection matrices.
*/
class Camera
{
public:
    Camera(glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, glm::vec3 yawPitchRoll, float nearViewPlane, float farViewPlane, glm::vec3 WorldUp);
    virtual ~Camera() { };

    // Camera (position & Euler angles)
    glm::vec3 camPos;   ///< camera position
    float yaw;          ///< camera yaw (deg)       Y|  R
    float pitch;        ///< camera pitch (deg)      | /
    float roll;         ///< camera roll (deg)       |/____P

    // View
    float fov, minFov, maxFov;  ///< FOV (deg)
    float nearViewPlane;        ///< Near view plane
    float farViewPlane;         ///< Near view plane

    /**
    * @brief Process all inputs that affect the camera (keyboard, mouse movement, mouse scroll).
    * @param deltaTime Time difference between the current frame and the previous frame.
    */
    void ProcessCameraInput(GLFWwindow* window, float deltaTime);

    /// Returns view matrix
    virtual glm::mat4 GetViewMatrix() = 0;

    /// Returns projection matrix (if it doesn't change each frame, it can be called outside the render loop)
    glm::mat4 GetProjectionMatrix(const float& aspectRatio);

    void setYScrollOffset(double yScrollOffset) { this->yScrollOffset = yScrollOffset; }
 
protected:
    // Camera vectors
    glm::vec3 worldUp;      //!< World up vector (used for computing camera's right vector) (got from up param. in constructor)
    glm::vec3 right;        //!< camera right vector
    glm::vec3 front;        //!< camera front vector
    glm::vec3 camUp;        //!< camera up vector (used in lookAt()) (computed from cross(right, front))

    // Controls
    float keysSpeed;        ///< camera speed
    float mouseSpeed;       ///< Mouse sensitivity
    float scrollSpeed;      ///< Scroll speed
    double yScrollOffset;   ///< Used for storing the mouse scroll offset
 
    // Helper variables:
    bool leftMousePressed;  ///< Used for checking which is the first frame with left mouse button pressed.
    double lastX, lastY;    ///< Mouse positions in the previous frame.
    double pi;

    /**
    * @brief Processes input received from any keyboard-like input system
    * @param deltaTime Time between one frame and the next
    */
    virtual void ProcessKeyboard(GLFWwindow* window, float deltaTime) = 0;

    /**
     * @brief Processes input received from a mouse input system
     * @param constrainPitch Limit camera's pitch movement (minimum and maximum value)
     */
    virtual void ProcessMouseMovement(GLFWwindow* window, float deltaTime) = 0;

    /**
     * @brief Processes input received from a mouse scroll-wheel event
     * @param yoffset Mouse scrolling value
     */
    virtual void ProcessMouseScroll(float deltaTime) = 0;

    /// Compute Front, Right and camera Up vector (from Euler angles and WorldUp)
    void updateCameraVectors();
};


class FreePolarCam : public Camera
{
public:
    FreePolarCam(glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, glm::vec2 yawPitch, float nearViewPlane, float farViewPlane, glm::vec3 worldUp);
    
    glm::mat4 FreePolarCam::GetViewMatrix();

private: 
    void ProcessKeyboard(GLFWwindow* window, float deltaTime) override;
    void ProcessMouseMovement(GLFWwindow* window, float deltaTime) override;
    void ProcessMouseScroll(float deltaTime) override;
};


class SphereCam : public Camera
{
public:
    SphereCam(float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, float nearViewPlane, float farViewPlane, glm::vec3 worldUp, glm::vec3 nucleus, float radius, float longitude, float latitude);

    glm::mat4 GetViewMatrix();

private:
    float radius;
    glm::vec3 nucleus;
    float latitude;
    float longitude;

    void ProcessKeyboard(GLFWwindow* window, float deltaTime) override;
    void ProcessMouseMovement(GLFWwindow* window, float deltaTime) override;
    void ProcessMouseScroll(float deltaTime) override;
};


class PlaneBasicCam : public Camera
{
public:
    //PlaneBasicCam(glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, glm::vec2 yawPitch, float nearViewPlane, float farViewPlane, glm::vec3 worldUp);

    //glm::mat4 FreePolarCam::GetViewMatrix();

private:
    //void ProcessKeyboard(GLFWwindow* window, float deltaTime) override;
    //void ProcessMouseMovement(GLFWwindow* window, float deltaTime) override;
    //void ProcessMouseScroll(float deltaTime) override;
};

#endif
