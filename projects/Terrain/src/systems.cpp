#include "systems.hpp"

void s_Engine::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif
    
    c_Engine* c_eng = (c_Engine*)em->getSComponent("engine");

    if (c_eng)
    {
        //c_eng->window = c_eng->r.getWindowManager();    // <<< Necessary to update each frame???
        c_eng->width = c_eng->r.getScreenSize().x;
        c_eng->height = c_eng->r.getScreenSize().y;
    }
    else std::cout << "No input component found: " << typeid(this).name() << std::endl;
}

void s_Input::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    c_Input*  c_input  = (c_Input* ) em->getSComponent("input");
    c_Engine* c_engine = (c_Engine*) em->getSComponent("engine");

    if (c_input && c_engine)
    {
        getKeyboardInput(c_engine->window, c_input, timeStep);
        getMouseInput(c_engine->window, c_input, timeStep);
    }
    else
        std::cout << "No input component found: " << typeid(this).name() << std::endl;
}

void s_Input::getKeyboardInput(GLFWwindow* window, c_Input* c_input, float timeStep)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) c_input->W_press = true;
    else c_input->W_press = false;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) c_input->S_press = true;
    else c_input->S_press = false;
        
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) c_input->A_press = true;
    else c_input->A_press = false;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) c_input->D_press = true;
    else c_input->D_press = false;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) c_input->Q_press = true;
    else c_input->Q_press = false;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) c_input->E_press = true;
    else c_input->E_press = false;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) c_input->up_press = true;
    else c_input->up_press = false;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) c_input->down_press = true;
    else c_input->down_press = false;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) c_input->left_press = true;
    else c_input->left_press = false;

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) c_input->right_press = true;
    else c_input->right_press = false;
}

void s_Input::getMouseInput(GLFWwindow* window, c_Input* c_input, float timeStep)
{
    c_input->LMB_justPressed = false;
    c_input->RMB_justPressed = false;
    c_input->MMB_justPressed = false;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (c_input->LMB_pressed == false)
            c_input->LMB_justPressed = true;

        c_input->LMB_pressed = true;
    }
    else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
        c_input->LMB_pressed = false;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        if (c_input->RMB_pressed == false)
            c_input->RMB_justPressed = true;

        c_input->RMB_pressed = true;
    }
    else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
        c_input->RMB_pressed = false;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
    {
        if (c_input->MMB_pressed == false)
            c_input->MMB_justPressed = true;

        c_input->MMB_pressed = true;
    }
    else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE)
        c_input->MMB_pressed = false;
    
    c_input->lastX = c_input->xpos;
    c_input->lastY = c_input->ypos;
    glfwGetCursorPos(window, &c_input->xpos, &c_input->ypos);

    c_input->yScrollOffset;     //!< Set in a callback (windowUserPointer)
}

glm::mat4 s_Camera::getViewMatrix(glm::vec3& camPos, glm::vec3& front, glm::vec3& camUp)
{
    return glm::lookAt(camPos, camPos + front, camUp);      // Params: Eye position, center position, up axis.
}

glm::mat4 s_Camera::getProjectionMatrix(float aspectRatio, float fov, float nearViewPlane, float farViewPlane)
{
    glm::mat4 proj = glm::perspective(fov, aspectRatio, nearViewPlane, farViewPlane);   // Params: FOV, aspect ratio, near and far view planes.
    proj[1][1] *= -1;       // GLM returns the Y clip coordinate inverted.
    return proj;
}

void s_PlaneCam::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    c_Input const* c_input = (c_Input*)em->getSComponent("input");
    c_Engine const* c_engine = (c_Engine*)em->getSComponent("engine");
    c_Camera* c_cam = (c_Camera*)em->getSComponent("camera");

    //if (c_input->LMB_pressed) std::cout << "X" << std::endl;
    //else std::cout << "." << std::endl;
    //if (c_input->LMB_justPressed) std::cout << "X<<<" << std::endl;

    if (!c_input || !c_engine || !c_cam)
    {
        std::cout << "No input component found: " << typeid(this).name() << std::endl;
        return;
    }
    
    float velocity = c_input->keysSpeed * timeStep;
    float yaw   = 0;					//!< cam yaw       Y|  R
    float pitch = 0;				    //!< cam pitch      | /
    float roll  = 0;					//!< cam roll       |/____P
    
    // Keyboard moves
    if (c_input->W_press || c_input->up_press   ) c_cam->camPos += c_cam->front * velocity;
    if (c_input->S_press || c_input->down_press ) c_cam->camPos -= c_cam->front * velocity;
    if (c_input->A_press || c_input->left_press ) roll = -0.05 * velocity;
    if (c_input->D_press || c_input->right_press) roll =  0.05 * velocity;
    if (c_input->Q_press) yaw =  0.05 * velocity;
    if (c_input->E_press) yaw = -0.05 * velocity;
    
    // Mouse moves
    if (c_input->LMB_pressed) {
        yaw -= c_input->xOffset() * c_input->mouseSpeed;
        pitch -= c_input->yOffset() * c_input->mouseSpeed;
    }
    
    // Cursor modes
    if (c_input->LMB_justPressed)
        glfwSetInputMode(c_engine->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (!c_input->LMB_pressed)
        glfwSetInputMode(c_engine->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Mouse scroll
    std::cout << c_input->yScrollOffset << std::endl;
    if (c_input->yScrollOffset)
    {
        c_cam->fov -= (float)c_input->yScrollOffset * c_input->scrollSpeed;
        if (c_cam->fov < c_cam->minFov) c_cam->fov = c_cam->minFov;
        if (c_cam->fov > c_cam->maxFov) c_cam->fov = c_cam->maxFov;
    
        //c_input->yScrollOffset() = 0;     // <<<
    }
    
    // Update cam vectors
    glm::vec4 rotQuat = productQuat(
        getRotQuat(c_cam->front, roll),
        getRotQuat(c_cam->camUp, yaw),
        getRotQuat(c_cam->right, pitch)
    );
    c_cam->front = rotatePoint(rotQuat, c_cam->front);
    c_cam->right = rotatePoint(rotQuat, c_cam->right);
    c_cam->camUp = rotatePoint(rotQuat, c_cam->camUp);
    
    c_cam->right = glm::normalize(glm::cross(c_cam->front, c_cam->camUp));
    c_cam->camUp = glm::normalize(glm::cross(c_cam->right, c_cam->front));
    c_cam->front = glm::normalize(c_cam->front);
    
    // Update View & Projection transformation matrices
    c_cam->view = getViewMatrix(c_cam->camPos, c_cam->front, c_cam->camUp);
    c_cam->proj = getProjectionMatrix(c_engine->aspectRatio(), c_cam->fov, c_cam->nearViewPlane, c_cam->farViewPlane);
}

void s_Model::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

}

void s_ModelMatrix::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    std::vector<uint32_t> entities = em->getEntitySet("mm");
    c_ModelMatrix* c_mm;
    c_Position* c_pos;

    for (uint32_t eId : entities)
    {
        c_mm = (c_ModelMatrix*)em->getComponent("mm", eId);
        c_pos = (c_Position*)em->getComponent("position", eId);
        if (c_pos) c_mm->translation = c_pos->pos;
        c_mm->modelMatrix = modelMatrix2(c_mm->scaling, c_mm->rotQuat, c_mm->translation);
    }
}

void s_Position::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    std::vector<uint32_t> entities = em->getEntitySet("position");
    c_Position* c_pos;
    c_Camera* c_cam;

    for (uint32_t eId : entities)
    {
        c_pos = (c_Position*)em->getComponent("position", eId);
        
        if(c_pos)
            if (c_pos->followCam)
            {
                c_cam = (c_Camera*)em->getSComponent("camera");
                c_pos->pos = c_cam->camPos;
            }
    }
}

void s_UBO::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    std::vector<uint32_t> entities = em->getEntitySet("model");
    c_Model* c_model;
    c_ModelMatrix* c_mm;
    c_Camera* c_cam;

    uint8_t* dest;
    int i;
    
    for (uint32_t eId : entities)
    {
        c_model = (c_Model*)em->getComponent("model", eId);
        c_mm    = (c_ModelMatrix*)em->getComponent("mm", eId);
        c_cam   = (c_Camera*)em->getSComponent("camera");

        if (c_mm && c_cam)
            for (i = 0; i < c_model->model->vsDynUBO.numDynUBOs; i++)
            {
                dest = c_model->model->vsDynUBO.getUBOptr(i);
                memcpy(dest + 0 * size.mat4, &c_mm->modelMatrix, sizeof(c_mm->modelMatrix));
                memcpy(dest + 1 * size.mat4, &c_cam->view, sizeof(c_cam->view));
                memcpy(dest + 2 * size.mat4, &c_cam->proj, sizeof(c_cam->proj));
            }
    }
}