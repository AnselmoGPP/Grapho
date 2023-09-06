#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP

#include <iostream>
#include <cmath>
#include <map>
#include <list>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "toolkit.hpp"
#include "ECSarch.hpp"

#include "noise.hpp"
#include "common.hpp"
#include "components.hpp"


//#define DEBUG_SYSTEM

// Singletons --------------------------------------

class s_Engine : public System
{
public:
    s_Engine() : System() { };
    ~s_Engine() { };

    void update(float timeStep) override;   //!< Update engine (c_Engine)
};

class s_Input : public System
{
    void getKeyboardInput(IOmanager* io, c_Input* c_input, float deltaTime);
    void getMouseInput(IOmanager* io, c_Input* c_input, float deltaTime);

public:
    s_Input() : System() { };
    ~s_Input() { };

    void update(float timeStep) override;   //!< Update input data (c_Input)
};

class s_Camera : public System
{
protected:
    glm::mat4 getViewMatrix(glm::vec3& camPos, glm::vec3& front, glm::vec3&camUp);
    glm::mat4 getProjectionMatrix(float aspectRatio, float fov, float nearViewPlane, float farViewPlane);
    void updateAxes(c_Camera* c_cam, glm::vec4& rotQuat);                   //!< Rotate current axes.

public:
    s_Camera() : System() { };
    ~s_Camera() { };

    virtual void update(float timeStep) = 0;
};

class s_SphereCam : public s_Camera
{
public:
    s_SphereCam() : s_Camera() { };
    ~s_SphereCam() { };

    void update(float timeStep) override;       //!< Update camera (c_Camera) & engine::GLFWwindow (c_Engine)
};

class s_PolarCam : public s_Camera
{
public:
    s_PolarCam() : s_Camera() { };
    ~s_PolarCam() { };

    void update(float timeStep) override;       //!< Update camera (c_Camera) & engine::GLFWwindow (c_Engine)
};

class s_PlaneCam : public s_Camera
{
public:
    s_PlaneCam() : s_Camera() { };
    ~s_PlaneCam() { };

    void update(float timeStep) override;       //!< Update camera (c_Camera) & engine::GLFWwindow (c_Engine)
};

class s_FPCam : public s_Camera
{
public:
    s_FPCam() : s_Camera() { };
    ~s_FPCam() { };

    void update(float timeStep) override;       //!< Update camera (c_Camera) & engine::GLFWwindow (c_Engine)
};


// Non-Singletons --------------------------------------

class s_Model : public System
{
public:
    s_Model() : System() { };
    ~s_Model() { };

    void update(float timeStep) override;       //!< <<< NOT DEFINED
};

class s_ModelMatrix : public System
{
public:
    s_ModelMatrix() : System() { };
    ~s_ModelMatrix() { };

    void update(float timeStep) override;       //!< Update Model Matrix (c_ModelMatrix)
};

class s_Move : public System
{
public:
    s_Move() : System() { };
    ~s_Move() { };

    void update(float timeStep) override;       //!< Update position vector (c_Position)
};

class s_UBO : public System
{
public:
    s_UBO() : System() { };
    ~s_UBO() { };

    void update(float timeStep) override;       //!< Update UBO (c_UBO)
};

class s_Sun : public System
{
public:
    s_Sun() : System() { };
    ~s_Sun() { };

    void update(float timeStep) override;       //!< Update UBO (c_UBO)
};

#endif
