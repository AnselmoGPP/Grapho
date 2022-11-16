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
    front(1, 0, 0), right(0, -1, 0), camUp(0, 0, 1),
    fov(fov), minFov(minFov), maxFov(maxFov),
    nearViewPlane(nearViewPlane), farViewPlane(farViewPlane),
    keysSpeed(keysSpeed), mouseSpeed(mouseSpeed), scrollSpeed(scrollSpeed),
    yScrollOffset(0),
    leftMousePressed(false),
    pi(3.141592653589793238462) { }

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

void Camera::updateCameraVectors()
{
    // Front vector
    front.x = cos(yaw) * cos(pitch);            // Calculation: Rotate direction vectors from (1,0,0) to their correct positions using the Euler angles
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


// FreePolarCam ---------------------------------------------

FreePolarCam::FreePolarCam(glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, glm::vec2 yawPitch, float nearViewPlane, float farViewPlane, glm::vec3 worldUp)
    : Camera(camPos, keysSpeed, mouseSpeed, scrollSpeed, fov, minFov, maxFov, glm::vec3(yawPitch.x, yawPitch.y, 0.f), nearViewPlane, farViewPlane), worldUp(worldUp)
{ 
    updateCameraVectors();
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


// PlaneCam ---------------------------------------------

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


// PlaneCam2 ---------------------------------------------

PlaneCam2::PlaneCam2(glm::vec3 camPos, float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, glm::vec3 yawPitchRoll, float nearViewPlane, float farViewPlane)
    : Camera(camPos, keysSpeed, mouseSpeed, scrollSpeed, fov, minFov, maxFov, yawPitchRoll, nearViewPlane, farViewPlane)
{
    front = { 1, 0, 0 };
    right = { 0,-1, 0 };
    camUp = { 0, 0, 1 };
    updateCameraVectors();
}

void PlaneCam2::ProcessKeyboard(GLFWwindow* window, float deltaTime)
{
    float velocity = keysSpeed * deltaTime;
    //roll = yaw = pitch = 0;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) camPos +=  front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) camPos -=  front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) roll   +=  0.05  * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) roll   += -0.05  * velocity;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)                                                     yaw    +=  0.05  * velocity;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)                                                     yaw    += -0.05  * velocity;
}

void PlaneCam2::ProcessMouseMovement(GLFWwindow* window, float deltaTime)
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
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        leftMousePressed = false;
    }
}

void PlaneCam2::ProcessMouseScroll(float deltaTime)
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

SphereCam::SphereCam(float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, float nearViewPlane, float farViewPlane, glm::vec3 worldUp, glm::vec3 nucleus, float radius, float latitude, float longitude)
    : Camera(glm::vec3(0,0,0), keysSpeed, mouseSpeed, scrollSpeed, fov, minFov, maxFov, glm::vec3(0, 0, 0), nearViewPlane, farViewPlane), nucleus(nucleus), radius(radius), latitude(glm::radians(latitude)), longitude(glm::radians(longitude)), worldUp(worldUp)
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


// PlanetFPcam ---------------------------------------------

PlanetFPcam::PlanetFPcam(float keysSpeed, float mouseSpeed, float scrollSpeed, float fov, float minFov, float maxFov, float nearViewPlane, float farViewPlane, glm::vec3 nucleus, float radius, float latitude, float longitude)
    : Camera(camPos, keysSpeed, mouseSpeed, scrollSpeed, fov, minFov, maxFov, glm::vec3(0,0,0), nearViewPlane, farViewPlane), nucleus(nucleus), radius(radius), camParticle(glm::vec3(0,0,0), glm::vec3(0, 0, 0), 0.f, 9.8, nucleus), spaceKey(GLFW_KEY_SPACE), onFloor(false), maxAngle(glm::radians(80.f))
{
    // Transform camera position coordinates from polar to cartesian.
    float lat = glm::radians(latitude);
    float lon = glm::radians(longitude);

    camPos = {
        cos(lat) * cos(lon) * radius + nucleus.x,
        cos(lat) * sin(lon) * radius + nucleus.y,
        sin(lat) * radius + nucleus.z };

    camParticle.setPos(camPos); // <<< Replace camPos (and maybe others) with camParticle
    //camParticle.setCallback(getFloorHeight);

    // Orientate camera (always starts pointing north)
    worldUp = glm::normalize(camPos - nucleus);
    
    glm::vec4 rotQuat = productQuat(
        getRotQuat(glm::vec3(1, 0, 0), lon),            // roll
        getRotQuat(glm::vec3(0, 0, 1), pi),             // yaw
        getRotQuat(glm::vec3(0, 1, 0), pi / 2 - lat)    // pitch
    );
    front = rotatePoint(rotQuat, front);
    right = rotatePoint(rotQuat, right);
    camUp = rotatePoint(rotQuat, camUp);
}

void PlanetFPcam::updateCameraVectors()
{
    //Camera::updateCameraVectors();
    //worldUp = glm::normalize(camPos - nucleus);
}

void PlanetFPcam::ProcessKeyboard(GLFWwindow* window, float deltaTime)
{
    glm::vec3 moveDir(0.f, 0.f, 0.f);
    glm::vec3 jumpVec(0.f, 0.f, 0.f);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        moveDir += glm::normalize(glm::cross(worldUp, right));
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        moveDir -= glm::normalize(glm::cross(worldUp, right));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        moveDir -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        moveDir += right;
    if (spaceKey.isFirstPress(window) && onFloor) 
        jumpVec = worldUp * 10.f;
    //if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS);// yaw +=  0.05 * velocity;
    //if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);// yaw += -0.05 * velocity;

    if (!moveDir.x && !moveDir.y && !moveDir.z) {
        if (!jumpVec.x && !jumpVec.y && !jumpVec.z && onFloor) return;  // Keeps camPos still when no move is made
    }
    else moveDir = glm::normalize(moveDir);

    // Get new camera position
    camParticle.setSpeedNP(keysSpeed * moveDir);
    camParticle.setSpeedP(jumpVec);
    camParticle.updateState(deltaTime);       // <<< Altitude should be computed after position calculation
    glm::vec3 newCamPos = camParticle.getPos();
    onFloor = camParticle.isOnFloor();

    // Rotate camera axes (keyboard)
    if (camPos != newCamPos)
    {
        glm::vec4 rotQuat = getRotQuat(glm::cross(worldUp, moveDir), angleBetween(camPos, newCamPos, nucleus));
        worldUp = glm::normalize(newCamPos - nucleus);
        camPos = newCamPos;
        
        front = rotatePoint(rotQuat, front);
        right = glm::normalize(glm::cross(front, worldUp)); // Computed using cross product to ensure that right axis is perpendicular to worldUp (otherwise, cumulative errors during rotations may cause disalignment with worldUp), and to ensure perpendicularity between the 3 camera axes (front, right, camUp).
        camUp = glm::normalize(glm::cross(right, front));
    }
}

void PlanetFPcam::ProcessMouseMovement(GLFWwindow* window, float deltaTime)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)   // if LMB pressed
    {
        if (leftMousePressed == false)  // if first time LMB is pressed
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window, &lastX, &lastY);
            leftMousePressed = true;
        }

        // Get offsets (xoffset is + to right, - to left) (yoffset is + downwards, - upwards)
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        double xoffset = xpos - lastX;
        double yoffset = ypos - lastY;
        lastX = xpos;
        lastY = ypos;

        // Check pitch limits
        float currentPitchAngle = asin(glm::dot(front, worldUp));
        float nextPitchAngle = currentPitchAngle - yoffset * mouseSpeed;
        if (nextPitchAngle > maxAngle) yoffset = (yoffset * mouseSpeed + (nextPitchAngle - maxAngle)) / mouseSpeed;
        else if (nextPitchAngle < -maxAngle) yoffset = (yoffset * mouseSpeed + (nextPitchAngle + maxAngle)) / mouseSpeed;

        // Rotate camera axes (mouse)
        glm::vec4 rotQuat = productQuat(
                getRotQuat(-worldUp, xoffset * mouseSpeed),     // worldUp rotation (Yaw)
                getRotQuat(-right, yoffset * mouseSpeed)        // right rotation (Pitch)
            );
        front = rotatePoint(rotQuat, front);
        right = glm::normalize(glm::cross(front, worldUp));
        camUp = glm::normalize(glm::cross(right, front));
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)    // if LMB released
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        leftMousePressed = false;
    }
}

void PlanetFPcam::ProcessMouseScroll(float deltaTime)
{
    if (yScrollOffset != 0)
    {
        fov -= (float)yScrollOffset * scrollSpeed;
        if (fov < minFov) fov = minFov;
        if (fov > maxFov) fov = maxFov;

        yScrollOffset = 0;
    }
}


// PressBegin ---------------------------------------------

PressBegin::PressBegin(int GLFW_KEY) : GLFW_KEY(GLFW_KEY), wasPressed(false) { }

bool PressBegin::isFirstPress(GLFWwindow* window)
{
    int keyState = glfwGetKey(window, GLFW_KEY) == GLFW_PRESS;
    
    if(!wasPressed && keyState == GLFW_PRESS)
    {
        wasPressed = true;
        return true;
    }
    
    if (keyState == GLFW_RELEASE)
    {
        wasPressed = false;
    }
        

    return false;
}