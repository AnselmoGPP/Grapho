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
