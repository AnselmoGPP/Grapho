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
    void getKeyboardInput(IOmanager& io, c_Input* c_input, float deltaTime);
    void getMouseInput(IOmanager& io, c_Input* c_input, float deltaTime);

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

    void update_Sphere(float timeStep, c_Camera* c_cam);
    void update_Plane_polar(float timeStep, c_Camera* c_cam);
    void update_Plane_free(float timeStep, c_Camera* c_cam);
    void update_FPV(float timeStep, c_Camera* c_cam);

public:
    s_Camera() : System() { };
    ~s_Camera() { };

    void update(float timeStep);        //!< Update camera (c_Camera) & engine::GLFWwindow (c_Engine)
};

class s_Lights : public System
{
public:
    s_Lights() : System() { };
    ~s_Lights() { };

    void update(float timeStep) override;
};

/// Update c_Sky (sky & sun rotation)
class s_Sky_XY : public System
{
public:
    s_Sky_XY() : System() { };
    ~s_Sky_XY() { };

    void update(float timeStep) override;
};


// Non-Singletons --------------------------------------

/// Update Model Matrix (c_ModelMatrix) using c_Move (position & rotation) and c_ModelMatrix (scale)
class s_ModelMatrix : public System
{
public:
    s_ModelMatrix() : System() { };
    ~s_ModelMatrix() { };

    void update(float timeStep) override;
};

/// Update c_Move (position & rotation)
class s_Move : public System
{
    void updateSkyMove(c_Move* c_mov, const c_Camera* c_cam, float angle, float dist);

public:
    s_Move() : System() { };
    ~s_Move() { };

    void update(float timeStep) override;
};

/// Update c_UBO
class s_Model : public System
{
public:
    s_Model() : System() { };
    ~s_Model() { };

    void update(float timeStep) override;
};

/// Update XXX
class s_Template : public System
{
public:
    s_Template() : System() { };
    ~s_Template() { };

    void update(float timeStep) override { };
};

#endif
