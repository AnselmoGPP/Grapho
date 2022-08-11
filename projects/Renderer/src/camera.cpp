/*
    Right axis: Represents the positive x-axis of camera space. First, we specify a vector pointing upwards (in world space), 
    then we do a cross product on the up vector and vector direction, resulting in a vector perpendicular to both and pointing 
    in the positive x-axis.

        glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));

    Up axis: Cross product of the right and direction vector.

        glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

    Cross & Dot product: https://www.youtube.com/watch?v=h0NJK4mEIJU

    Camera class:
        1. ProcessCameraInput
            - ProcessKeyboard
            - ProcessMouseMovement
            - ProcessMouseScroll
        2. GetViewMatrix
        3. GetProjectionMatrix
*/
#include <iostream>
#include "camera.hpp"


// Camera ---------------------------------------------

Camera::Camera(glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, glm::vec3 yawPitchRoll, float nearViewPlane, float farViewPlane)
    : camPos(camPos),
    yaw(glm::radians(yawPitchRoll.x)), pitch(glm::radians(yawPitchRoll.y)), roll(glm::radians(yawPitchRoll.z)),
    fov(fov), minFov(minFov), maxFov(maxFov),
    nearViewPlane(nearViewPlane), farViewPlane(farViewPlane),
    keysSpeed(keysSpeed), mouseSpeed(mouseSpeed), scrollSpeed(scrollSpeed),
    yScrollOffset(0),
    leftMousePressed(false),
    pi(3.14159265359) { }

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(camPos, camPos + front, camUp);      // Params: Eye position, center position, up axis.
}

glm::mat4 Camera::GetProjectionMatrix(const float& aspectRatio)
{
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearViewPlane, farViewPlane);   // Params: FOV, aspect ratio, near and far view planes.
    proj[1][1] *= -1;       // GLM returns the Y clip coordinate inverted.

    return proj;
}

void Camera::ProcessCameraInput(GLFWwindow* window, float deltaTime)
{
    ProcessKeyboard(window, deltaTime);
    ProcessMouseMovement(window, deltaTime);
    ProcessMouseScroll(deltaTime);

    updateCameraVectors();
}


// FreePolarCam ---------------------------------------------

FreePolarCam::FreePolarCam(glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, glm::vec2 yawPitch, float nearViewPlane, float farViewPlane, glm::vec3 worldUp)
    : Camera(camPos, keysSpeed, mouseSpeed, scrollSpeed, fov, minFov, maxFov, glm::vec3(yawPitch.x, yawPitch.y, 0.f), nearViewPlane, farViewPlane), worldUp(worldUp)
{ 
    updateCameraVectors();
}

void FreePolarCam::updateCameraVectors()
{
    // Front vector
    front.x = cos(yaw) * cos(pitch);
    front.y = sin(yaw) * cos(pitch);
    front.z = sin(pitch);
    front = glm::normalize(front);

    // Right vector
    right.x = cos(yaw - pi / 2) * cos(roll);    //Another option: right = glm::normalize(glm::cross(front, worldUp));
    right.y = sin(yaw - pi / 2) * cos(roll);
    right.z = sin(roll);
    right = glm::normalize(right);

    // Camera Up vector
    camUp = glm::normalize(glm::cross(right, front));
}

void FreePolarCam::ProcessKeyboard(GLFWwindow* window, float deltaTime)
{
    float velocity = keysSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) camPos += front   * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) camPos -= front   * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT ) == GLFW_PRESS) camPos -= right   * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camPos += right   * velocity;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)                                                     camPos -= worldUp * velocity;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)                                                     camPos += worldUp * velocity;
}

void FreePolarCam::ProcessMouseMovement(/*float xoffset, float yoffset, */GLFWwindow* window, float deltaTime)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (leftMousePressed == false)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window, &lastX, &lastY);
            leftMousePressed = true;
        }
        
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        double xoffset = xpos - lastX;
        double yoffset = ypos - lastY;
        lastX = xpos;
        lastY = ypos;

        yaw -= xoffset * mouseSpeed;
        pitch -= yoffset * mouseSpeed;

        if (true)   // constrain pitch
        {
            if (pitch >  (pi-0.01)/2) pitch =  (pi - 0.01) /2;
            if (pitch < -(pi-0.01)/2) pitch = -(pi - 0.01) /2;
        }

        //updateCameraVectors();
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        leftMousePressed = false;
    }
}

void FreePolarCam::ProcessMouseScroll(float deltaTime)
{
    if (yScrollOffset != 0)
    {
        fov -= (float)yScrollOffset * scrollSpeed;
        if (fov < minFov) fov = minFov;
        if (fov > maxFov) fov = maxFov;

        yScrollOffset = 0;
    }
}


// PlaneBasicCam ---------------------------------------------

PlaneCam::PlaneCam(glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, glm::vec3 yawPitchRoll, float nearViewPlane, float farViewPlane)
    : Camera(camPos, keysSpeed, mouseSpeed, scrollSpeed, fov, minFov, maxFov, yawPitchRoll, nearViewPlane, farViewPlane)
{
    front = { 1, 0, 0 };
    right = { 0,-1, 0 };
    camUp = { 0, 0, 1 };
    updateCameraVectors();
}

void PlaneCam::updateCameraVectors()
{
    // Yaw changes
    right = cos(yaw) * right + sin(yaw) * front;
    front = glm::cross(camUp, right);

    // Pitch changes
    front = cos(pitch) * front + sin(pitch) * camUp;
    camUp = glm::cross(right, front);

    // Roll changes
    right = cos(roll) * right + sin(roll) * camUp;
    camUp = glm::cross(right, front);

    // Normalization
    right = glm::normalize(right);
    front = glm::normalize(front);
    camUp = glm::normalize(camUp);
}

void PlaneCam::ProcessKeyboard(GLFWwindow* window, float deltaTime)
{
    float velocity = keysSpeed * deltaTime;
    roll = yaw = pitch = 0;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) camPos += front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) camPos -= front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) roll    =  0.05 * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) roll    = -0.05 * velocity;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)                                                     yaw     =  0.05 * velocity;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)                                                     yaw     = -0.05 * velocity;
}

void PlaneCam::ProcessMouseMovement(GLFWwindow* window, float deltaTime)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (leftMousePressed == false)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window, &lastX, &lastY);
            leftMousePressed = true;
        }

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        double xoffset = xpos - lastX;
        double yoffset = ypos - lastY;
        lastX = xpos;
        lastY = ypos;

        yaw   -= xoffset * mouseSpeed;
        pitch -= yoffset * mouseSpeed;
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        leftMousePressed = false;
    }
}

void PlaneCam::ProcessMouseScroll(float deltaTime)
{
    if (yScrollOffset != 0)
    {
        fov -= (float)yScrollOffset * scrollSpeed;
        if (fov < minFov) fov = minFov;
        if (fov > maxFov) fov = maxFov;

        yScrollOffset = 0;
    }
}


// SphereCam ---------------------------------------------

SphereCam::SphereCam(float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, float nearViewPlane, float farViewPlane, glm::vec3 worldUp, glm::vec3 nucleus, float radius, float longitude, float latitude)
    : Camera(glm::vec3(0,0,0), keysSpeed, mouseSpeed, scrollSpeed, fov, minFov, maxFov, glm::vec3(0, 0, 0), nearViewPlane, farViewPlane), nucleus(nucleus), radius(radius), longitude(glm::radians(longitude)), latitude(glm::radians(latitude)), worldUp(worldUp)
{
    // Transform coordinates from polar to cartesian.
    camPos = { 
        cos(this->latitude) * cos(this->longitude) * radius + nucleus.x,
        cos(this->latitude) * sin(this->longitude) * radius + nucleus.y,
        sin(this->latitude) * radius + nucleus.z };
    
    // Flight axis
    yaw = this->longitude + pi;
    pitch = - this->latitude;
    roll = 0.f;

    // Front, Right, CamUp
    updateCameraVectors();
}

glm::mat4 SphereCam::GetViewMatrix()
{
    return glm::lookAt(camPos, nucleus, camUp);      // Params: Eye position, center position, up axis.
}

void SphereCam::updateCameraVectors()
{
    // Front vector
    front.x = cos(yaw) * cos(pitch);
    front.y = sin(yaw) * cos(pitch);
    front.z = sin(pitch);
    front = glm::normalize(front);

    // Right vector
    right.x = cos(yaw - pi / 2) * cos(roll);    //Another option: right = glm::normalize(glm::cross(front, worldUp));
    right.y = sin(yaw - pi / 2) * cos(roll);
    right.z = sin(roll);
    right = glm::normalize(right);

    // Camera Up vector
    camUp = glm::normalize(glm::cross(right, front));
}

void SphereCam::ProcessKeyboard(GLFWwindow* window, float deltaTime)
{
    float velocity = keysSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        glm::vec4 temp = glm::vec4(camPos, radius);

        camPos += front * velocity;
        glm::vec3 radiusVec(camPos - nucleus);
        radius = std::sqrt(std::pow(radiusVec.x, 2) + std::pow(radiusVec.y, 2) + std::pow(radiusVec.z, 2));

        if (radius < 10.f) { 
            camPos = glm::vec3(temp[0], temp[1], temp[2]);
            radius = temp[3];
        }
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        camPos -= front * velocity;
        glm::vec3 radiusVec(camPos - nucleus);
        radius = std::sqrt(std::pow(radiusVec.x, 2) + std::pow(radiusVec.y, 2) + std::pow(radiusVec.z, 2));
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        camPos -= right * velocity;
        nucleus = camPos + front * radius;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        camPos += right * velocity;
        nucleus = camPos + front * radius;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        camPos -= worldUp * velocity;
        nucleus = camPos + front * radius;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        camPos += worldUp * velocity;
        nucleus = camPos + front * radius;
    }
}

void SphereCam::ProcessMouseMovement(/*float xoffset, float yoffset, */GLFWwindow* window, float deltaTime)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (leftMousePressed == false)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window, &lastX, &lastY);
            leftMousePressed = true;
        }

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        double xoffset = xpos - lastX;
        double yoffset = ypos - lastY;
        lastX = xpos;
        lastY = ypos;

        float oldLatitude = latitude;
        longitude -= xoffset * mouseSpeed;
        latitude += yoffset * mouseSpeed;
        if (std::abs(latitude) > pi / 2 - 0.01) latitude = oldLatitude;

        camPos = {
            cos(latitude) * cos(longitude) * radius + nucleus.x,
            cos(latitude) * sin(longitude) * radius + nucleus.y,
            sin(latitude) * radius + nucleus.z };

        yaw = longitude + pi;
        pitch = -latitude;

        updateCameraVectors();
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        leftMousePressed = false;
    }
}

void SphereCam::ProcessMouseScroll(float deltaTime)
{
    if (yScrollOffset != 0)
    {
        fov -= (float)yScrollOffset * scrollSpeed;
        if (fov < minFov) fov = minFov;
        if (fov > maxFov) fov = maxFov;

        yScrollOffset = 0;
    }
}

