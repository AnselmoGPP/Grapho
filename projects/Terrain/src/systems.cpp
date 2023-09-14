#include "systems.hpp"

void s_Engine::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif
    
    c_Engine* c_eng = (c_Engine*)em->getSComponent("engine");
    if (!c_eng) std::cout << "Component not found" << std::endl;

    c_eng->time += timeStep;
    c_eng->frameCount++;
}

void s_Input::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    c_Input*  c_inp  = (c_Input* ) em->getSComponent("input");
    c_Engine* c_eng = (c_Engine*) em->getSComponent("engine");

    if (c_inp && c_eng)
    {
        // KEYBOARD input:

        if (c_eng->io.getKey(GLFW_KEY_W) == GLFW_PRESS) c_inp->W_press = true;
        else c_inp->W_press = false;

        if (c_eng->io.getKey(GLFW_KEY_S) == GLFW_PRESS) c_inp->S_press = true;
        else c_inp->S_press = false;

        if (c_eng->io.getKey(GLFW_KEY_A) == GLFW_PRESS) c_inp->A_press = true;
        else c_inp->A_press = false;

        if (c_eng->io.getKey(GLFW_KEY_D) == GLFW_PRESS) c_inp->D_press = true;
        else c_inp->D_press = false;

        if (c_eng->io.getKey(GLFW_KEY_Q) == GLFW_PRESS) c_inp->Q_press = true;
        else c_inp->Q_press = false;

        if (c_eng->io.getKey(GLFW_KEY_E) == GLFW_PRESS) c_inp->E_press = true;
        else c_inp->E_press = false;

        if (c_eng->io.getKey(GLFW_KEY_UP) == GLFW_PRESS) c_inp->up_press = true;
        else c_inp->up_press = false;

        if (c_eng->io.getKey(GLFW_KEY_DOWN) == GLFW_PRESS) c_inp->down_press = true;
        else c_inp->down_press = false;

        if (c_eng->io.getKey(GLFW_KEY_LEFT) == GLFW_PRESS) c_inp->left_press = true;
        else c_inp->left_press = false;

        if (c_eng->io.getKey(GLFW_KEY_RIGHT) == GLFW_PRESS) c_inp->right_press = true;
        else c_inp->right_press = false;

        // MOUSE input:

        c_inp->LMB_justPressed = false;
        c_inp->RMB_justPressed = false;
        c_inp->MMB_justPressed = false;

        if (c_eng->io.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            if (c_inp->LMB_pressed == false)
                c_inp->LMB_justPressed = true;

            c_inp->LMB_pressed = true;
        }
        else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
            c_inp->LMB_pressed = false;

        if (c_eng->io.getMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        {
            if (c_inp->RMB_pressed == false)
                c_inp->RMB_justPressed = true;

            c_inp->RMB_pressed = true;
        }
        else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
            c_inp->RMB_pressed = false;

        if (c_eng->io.getMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
        {
            if (c_inp->MMB_pressed == false)
                c_inp->MMB_justPressed = true;

            c_inp->MMB_pressed = true;
        }
        else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE)
            c_inp->MMB_pressed = false;

        c_inp->lastX = c_inp->xpos;
        c_inp->lastY = c_inp->ypos;
        c_eng->io.getCursorPos(&c_inp->xpos, &c_inp->ypos);

        c_inp->yScrollOffset = c_eng->io.getYscrollOffset();
    }
    else
        std::cout << "No input component found: " << typeid(this).name() << std::endl;
}

void s_Input::getKeyboardInput(IOmanager& io, c_Input* c_input, float timeStep)
{
    if (io.getKey(GLFW_KEY_W) == GLFW_PRESS) c_input->W_press = true;
    else c_input->W_press = false;

    if (io.getKey(GLFW_KEY_S) == GLFW_PRESS) c_input->S_press = true;
    else c_input->S_press = false;
        
    if (io.getKey(GLFW_KEY_A) == GLFW_PRESS) c_input->A_press = true;
    else c_input->A_press = false;

    if (io.getKey(GLFW_KEY_D) == GLFW_PRESS) c_input->D_press = true;
    else c_input->D_press = false;

    if (io.getKey(GLFW_KEY_Q) == GLFW_PRESS) c_input->Q_press = true;
    else c_input->Q_press = false;

    if (io.getKey(GLFW_KEY_E) == GLFW_PRESS) c_input->E_press = true;
    else c_input->E_press = false;

    if (io.getKey(GLFW_KEY_UP) == GLFW_PRESS) c_input->up_press = true;
    else c_input->up_press = false;

    if (io.getKey(GLFW_KEY_DOWN) == GLFW_PRESS) c_input->down_press = true;
    else c_input->down_press = false;

    if (io.getKey(GLFW_KEY_LEFT) == GLFW_PRESS) c_input->left_press = true;
    else c_input->left_press = false;

    if (io.getKey(GLFW_KEY_RIGHT) == GLFW_PRESS) c_input->right_press = true;
    else c_input->right_press = false;
}

void s_Input::getMouseInput(IOmanager& io, c_Input* c_input, float timeStep)
{
    c_input->LMB_justPressed = false;
    c_input->RMB_justPressed = false;
    c_input->MMB_justPressed = false;

    if (io.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (c_input->LMB_pressed == false)
            c_input->LMB_justPressed = true;

        c_input->LMB_pressed = true;
    }
    else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
        c_input->LMB_pressed = false;

    if (io.getMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        if (c_input->RMB_pressed == false)
            c_input->RMB_justPressed = true;

        c_input->RMB_pressed = true;
    }
    else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
        c_input->RMB_pressed = false;

    if (io.getMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
    {
        if (c_input->MMB_pressed == false)
            c_input->MMB_justPressed = true;

        c_input->MMB_pressed = true;
    }
    else // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE)
        c_input->MMB_pressed = false;
    
    c_input->lastX = c_input->xpos;
    c_input->lastY = c_input->ypos;
    io.getCursorPos(&c_input->xpos, &c_input->ypos);

    c_input->yScrollOffset = io.getYscrollOffset();
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

void s_Camera::updateAxes(c_Camera* c_cam, glm::vec4& rotQuat)
{
    // Rotate
    c_cam->front = rotatePoint(rotQuat, c_cam->front);
    c_cam->right = rotatePoint(rotQuat, c_cam->right);
    c_cam->camUp = rotatePoint(rotQuat, c_cam->camUp);

    // Normalize & make perpendicular
    c_cam->right = glm::normalize(glm::cross(c_cam->front, c_cam->camUp));
    c_cam->camUp = glm::normalize(glm::cross(c_cam->right, c_cam->front));
    c_cam->front = glm::normalize(c_cam->front);
}

void s_SphereCam::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    const c_Input* c_input = (c_Input*)em->getSComponent("input");
    const c_Engine* c_engine = (c_Engine*)em->getSComponent("engine");
    c_Camera* c_cam = (c_Camera*)em->getSComponent("camera");

    if (!c_input || !c_engine || !c_cam) {
        std::cout << "No input component found: " << typeid(this).name() << std::endl;
        return;
    }
    
    // Cursor modes
    if (c_input->LMB_justPressed)
        c_engine->io.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!c_input->LMB_pressed)
        c_engine->io.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Keyboard moves
    float velocity = c_cam->keysSpeed * timeStep;
    float& yaw = c_cam->euler.z;				//!< cam yaw       Y|  R
    float& pitch = c_cam->euler.x;				//!< cam pitch      | /
    //float& roll = c_cam->euler.z;				//!< cam roll       |/____P

    if (c_input->W_press || c_input->up_press) c_cam->radius -= velocity;
    if (c_input->S_press || c_input->down_press) c_cam->radius += velocity;
    if (c_input->A_press || c_input->left_press) c_cam->center -= c_cam->right * velocity;
    if (c_input->D_press || c_input->right_press) c_cam->center += c_cam->right * velocity;
    if (c_input->Q_press) c_cam->center -= c_cam->camUp * velocity;
    if (c_input->E_press) c_cam->center += c_cam->camUp * velocity;

    // constrain Radius
    if (c_cam->radius < c_cam->minRadius) c_cam->radius = c_cam->minRadius;
    if (c_cam->radius > c_cam->maxRadius) c_cam->radius = c_cam->maxRadius;

    // Mouse moves
    if (c_input->LMB_pressed) {
        yaw -= c_input->xOffset() * c_cam->mouseSpeed;
        pitch -= c_input->yOffset() * c_cam->mouseSpeed;
    }
    
    // constrain Pitch
    if (abs(pitch) > c_cam->maxPitch)
        pitch > 0 ? pitch =  c_cam->maxPitch : pitch = -c_cam->maxPitch;

    // Mouse scroll
    if (c_input->yScrollOffset)
    {
        c_cam->fov -= (float)c_input->yScrollOffset * c_cam->scrollSpeed;
        if (c_cam->fov < c_cam->minFov) c_cam->fov = c_cam->minFov;
        if (c_cam->fov > c_cam->maxFov) c_cam->fov = c_cam->maxFov;
    }

    // Update cam vectors
    c_cam->front  = glm::vec3(-1, 0, 0);
    c_cam->right  = glm::vec3( 0, 1, 0);
    c_cam->camUp  = glm::vec3( 0, 0, 1);

    glm::vec4 rotQuat = productQuat(
        //getRotQuat(c_cam->front, roll),
        getRotQuat(c_cam->right, pitch),
        getRotQuat(c_cam->worldUp, yaw)
    );
    updateAxes(c_cam, rotQuat);

    // Update camPos
    c_cam->camPos = rotatePoint(rotQuat, glm::vec3(c_cam->radius, 0, 0));
    c_cam->camPos += c_cam->center;

    // Prevent error propagation
    c_cam->right = glm::normalize(glm::cross(c_cam->front, c_cam->worldUp));
    c_cam->camUp = glm::normalize(glm::cross(c_cam->right, c_cam->front));

    // Update View & Projection transformation matrices
    c_cam->view = getViewMatrix(c_cam->camPos, c_cam->front, c_cam->camUp);
    c_cam->proj = getProjectionMatrix(c_engine->aspectRatio(), c_cam->fov, c_cam->nearViewPlane, c_cam->farViewPlane);
}

void s_PolarCam::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    c_Input const* c_input = (c_Input*)em->getSComponent("input");
    c_Engine const* c_engine = (c_Engine*)em->getSComponent("engine");
    c_Camera* c_cam = (c_Camera*)em->getSComponent("camera");

    if (!c_input || !c_engine || !c_cam) {
        std::cout << "No input component found: " << typeid(this).name() << std::endl;
        return;
    }

    // Cursor modes
    if (c_input->LMB_justPressed)
        c_engine->io.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!c_input->LMB_pressed)
        c_engine->io.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Keyboard moves
    float velocity = c_cam->keysSpeed * timeStep;
    float& yaw = c_cam->euler.z;				//!< cam yaw       Y|  R
    float& pitch = c_cam->euler.x;				//!< cam pitch      | /
    //float& roll = c_cam->euler.z;				//!< cam roll       |/____P

    if (c_input->W_press || c_input->up_press) c_cam->camPos += c_cam->front * velocity;
    if (c_input->S_press || c_input->down_press) c_cam->camPos -= c_cam->front * velocity;
    if (c_input->A_press || c_input->left_press) c_cam->camPos -= c_cam->right * velocity;
    if (c_input->D_press || c_input->right_press) c_cam->camPos += c_cam->right * velocity;
    if (c_input->Q_press) c_cam->camPos -= c_cam->worldUp * velocity;
    if (c_input->E_press) c_cam->camPos += c_cam->worldUp * velocity;

    // Mouse moves
    if (c_input->LMB_pressed) {
        yaw -= c_input->xOffset() * c_cam->mouseSpeed;
        pitch -= c_input->yOffset() * c_cam->mouseSpeed;
    }

    if (abs(pitch) > 1.3) pitch > 0 ? pitch = 1.3 : pitch = -1.3;
    
    // Mouse scroll
    if (c_input->yScrollOffset)
    {
        c_cam->fov -= (float)c_input->yScrollOffset * c_cam->scrollSpeed;
        if (c_cam->fov < c_cam->minFov) c_cam->fov = c_cam->minFov;
        if (c_cam->fov > c_cam->maxFov) c_cam->fov = c_cam->maxFov;
    }

    // Update cam vectors
    c_cam->front = glm::vec3(1, 0, 0);
    c_cam->right = glm::vec3(0,-1, 0);
    c_cam->camUp = glm::vec3(0, 0, 1);

    glm::vec4 rotQuat = productQuat(
        //getRotQuat(c_cam->front, roll),
        getRotQuat(glm::vec3(0, -1, 0), pitch),
        getRotQuat(c_cam->worldUp, yaw)
    );
    updateAxes(c_cam, rotQuat);
    
    // Prevent error propagation
    c_cam->right = glm::normalize(glm::cross(c_cam->front, c_cam->worldUp));
    c_cam->camUp = glm::normalize(glm::cross(c_cam->right, c_cam->front));

    // Update View & Projection transformation matrices
    c_cam->view = getViewMatrix(c_cam->camPos, c_cam->front, c_cam->camUp);
    c_cam->proj = getProjectionMatrix(c_engine->aspectRatio(), c_cam->fov, c_cam->nearViewPlane, c_cam->farViewPlane);
}

void s_PlaneCam::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    c_Input const* c_input = (c_Input*)em->getSComponent("input");
    c_Engine const* c_engine = (c_Engine*)em->getSComponent("engine");
    c_Camera* c_cam = (c_Camera*)em->getSComponent("camera");

    if (!c_input || !c_engine || !c_cam) {
        std::cout << "No input component found: " << typeid(this).name() << std::endl;
        return;
    }

    // Cursor modes
    if (c_input->LMB_justPressed)
        c_engine->io.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!c_input->LMB_pressed)
        c_engine->io.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Keyboard moves
    float velocity = c_cam->keysSpeed * timeStep;
    float yaw   = 0;					//!< cam yaw       Y|  R
    float pitch = 0;				    //!< cam pitch      | /
    float roll  = 0;					//!< cam roll       |/____P
    
    if (c_input->W_press || c_input->up_press   ) c_cam->camPos += c_cam->front * velocity;
    if (c_input->S_press || c_input->down_press ) c_cam->camPos -= c_cam->front * velocity;
    if (c_input->A_press || c_input->left_press ) roll = -c_cam->spinSpeed * velocity;
    if (c_input->D_press || c_input->right_press) roll =  c_cam->spinSpeed * velocity;
    if (c_input->Q_press) yaw =  c_cam->spinSpeed * velocity;
    if (c_input->E_press) yaw = -c_cam->spinSpeed * velocity;
    
    // Mouse moves
    if (c_input->LMB_pressed) {
        yaw -= c_input->xOffset() * c_cam->mouseSpeed;
        pitch -= c_input->yOffset() * c_cam->mouseSpeed;
    }
    
    // Mouse scroll
    if (c_input->yScrollOffset)
    {
        c_cam->fov -= (float)c_input->yScrollOffset * c_cam->scrollSpeed;
        if (c_cam->fov < c_cam->minFov) c_cam->fov = c_cam->minFov;
        if (c_cam->fov > c_cam->maxFov) c_cam->fov = c_cam->maxFov;
    }
    
    // Update cam vectors
    glm::vec4 rotQuat = productQuat(
        getRotQuat(c_cam->front, roll),
        getRotQuat(c_cam->camUp, yaw),
        getRotQuat(c_cam->right, pitch)
    );
    updateAxes(c_cam, rotQuat);
    
    // Update View & Projection transformation matrices
    c_cam->view = getViewMatrix(c_cam->camPos, c_cam->front, c_cam->camUp);
    c_cam->proj = getProjectionMatrix(c_engine->aspectRatio(), c_cam->fov, c_cam->nearViewPlane, c_cam->farViewPlane);
}

void s_FPCam::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    c_Input const* c_input = (c_Input*)em->getSComponent("input");
    c_Engine const* c_engine = (c_Engine*)em->getSComponent("engine");
    c_Camera* c_cam = (c_Camera*)em->getSComponent("camera");

    if (!c_input || !c_engine || !c_cam) {
        std::cout << "No input component found: " << typeid(this).name() << std::endl;
        return;
    }

    // Cursor modes
    if (c_input->LMB_justPressed)
        c_engine->io.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!c_input->LMB_pressed)
        c_engine->io.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Keyboard moves
    float velocity = c_cam->keysSpeed * timeStep;
    float& yaw = c_cam->euler.z;				//!< cam yaw       Y|  R
    float& pitch = c_cam->euler.x;				//!< cam pitch      | /
    //float& roll = c_cam->euler.z;				//!< cam roll       |/____P

    if (c_input->W_press || c_input->up_press) c_cam->camPos += c_cam->front * velocity;
    if (c_input->S_press || c_input->down_press) c_cam->camPos -= c_cam->front * velocity;
    if (c_input->A_press || c_input->left_press) c_cam->camPos -= c_cam->right * velocity;
    if (c_input->D_press || c_input->right_press) c_cam->camPos += c_cam->right * velocity;
    if (c_input->Q_press) c_cam->camPos -= c_cam->worldUp * velocity;
    if (c_input->E_press) c_cam->camPos += c_cam->worldUp * velocity;

    // Mouse moves
    if (c_input->LMB_pressed) {
        yaw -= c_input->xOffset() * c_cam->mouseSpeed;
        pitch -= c_input->yOffset() * c_cam->mouseSpeed;
    }

    if (abs(pitch) > 1.3) pitch > 0 ? pitch = 1.3 : pitch = -1.3;

    // Mouse scroll
    if (c_input->yScrollOffset)
    {
        c_cam->fov -= (float)c_input->yScrollOffset * c_cam->scrollSpeed;
        if (c_cam->fov < c_cam->minFov) c_cam->fov = c_cam->minFov;
        if (c_cam->fov > c_cam->maxFov) c_cam->fov = c_cam->maxFov;
    }

    // Update cam vectors
    glm::vec4 rotQuat = productQuat(
        //getRotQuat(c_cam->front, roll),
        getRotQuat(glm::vec3(0, -1, 0), pitch),
        getRotQuat(c_cam->worldUp, yaw)
    );
    updateAxes(c_cam, rotQuat);

    // Prevent error propagation
    c_cam->right = glm::normalize(glm::cross(c_cam->front, c_cam->worldUp));
    c_cam->camUp = glm::normalize(glm::cross(c_cam->right, c_cam->front));

    // Update View & Projection transformation matrices
    c_cam->view = getViewMatrix(c_cam->camPos, c_cam->front, c_cam->camUp);
    c_cam->proj = getProjectionMatrix(c_engine->aspectRatio(), c_cam->fov, c_cam->nearViewPlane, c_cam->farViewPlane);
}

void s_Lights::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    c_Lights* c_lights = (c_Lights*)em->getSComponent("lights");
    if (!c_lights) return;

    unsigned count = c_lights->lights.numLights;
    unsigned i = 0;

    const c_Sky* c_sky = (c_Sky*)em->getSComponent("sky");
    if (!c_sky) return;

    if (i < count) c_lights->lights.posDir[i++].direction = -c_sky->sunDir;     // Dir. from sun

    if (i < count) c_lights->lights.posDir[i++].direction =  c_sky->sunDir;     // Dir. to sun

    if (i < count)
    {
        const c_Camera* c_cam = (c_Camera*)em->getSComponent("cam");
        if (!c_cam) return;

        c_lights->lights.posDir[i++].position  = c_cam->camPos;                 // Flashlight from cam
        c_lights->lights.posDir[i  ].direction = c_cam->front;
    }
}

void s_Sky_XY::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    c_Sky* c_sky = (c_Sky*)em->getSComponent("sky");
    if (!c_sky) return;

    c_Engine* c_eng = (c_Engine*)em->getSComponent("engine");
    if (!c_eng) return;
    
    c_sky->skyAngle = fmod(c_eng->time * c_sky->skySpeed + c_sky->skyAngle_0, 2 * pi);
    c_sky->sunAngle = fmod(c_eng->time * c_sky->sunSpeed + c_sky->sunAngle_0, 2 * pi);
    c_sky->sunDir = { cos(c_sky->sunAngle), sin(c_sky->sunAngle), 0 };
}

void s_ModelMatrix::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    std::vector<uint32_t> entities = em->getEntitySet("mm");
    c_ModelMatrix* c_mm;
    c_Move* c_mov;

    for (uint32_t eId : entities)
    {
        c_mm = (c_ModelMatrix*)em->getComponent("mm", eId);
        c_mov = (c_Move*)em->getComponent("move", eId);

        if (c_mov)
            c_mm->modelMatrix = getModelMatrix(c_mm->scale, c_mov->rotQuat, c_mov->pos);
    }
}

void s_Move::updateSkyMove(c_Move* c_mov, const c_Camera* c_cam, float angle, float dist)
{
    c_mov->pos.x = c_cam->camPos.x + cos(angle) * dist;
    c_mov->pos.y = c_cam->camPos.y + sin(angle) * dist;
    c_mov->pos.z = c_cam->camPos.z;

    c_mov->rotQuat = productQuat(
        getRotQuat(glm::vec3(0, 1, 0), 3 * pi / 2),
        getRotQuat(glm::vec3(0, 0, 1), angle));
}

void s_Move::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    std::vector<uint32_t> entities = em->getEntitySet("move");
    const c_Camera* c_cam;
    const c_Engine* c_eng;
    const c_Sky* c_sky;
    c_Move* c_mov;
    float angle;

    for (uint32_t eId : entities)
    {
        c_mov = (c_Move*)em->getComponent("move", eId);
        
        if (c_mov)
        {
            c_cam = (c_Camera*)em->getSComponent("camera");

            switch (c_mov->moveType)
            {
            case followCam:             // follow cam
                if (c_cam)
                    c_mov->pos = c_cam->camPos - safeMod(c_cam->camPos, c_mov->jumpStep);
                break;

            case followCamXY:           // follow cam on axis XY
                if (c_cam)
                    c_mov->pos = glm::vec3(
                        c_cam->camPos.x - safeMod(c_cam->camPos.x, c_mov->jumpStep),
                        c_cam->camPos.y - safeMod(c_cam->camPos.y, c_mov->jumpStep),
                        c_mov->pos.z );
                break;

            case skyOrbit:              // Update using c_Sky::skyAngle
                c_sky = (c_Sky*)em->getSComponent("sky");
                if (c_cam && c_sky)
                    updateSkyMove(c_mov, c_cam, c_sky->skyAngle, 0);
                break;

            case sunOrbit:              // Update using c_Sky::sunAngle
                c_sky = (c_Sky*)em->getSComponent("sky");
                if (c_cam && c_sky)
                    updateSkyMove(c_mov, c_cam, c_sky->sunAngle, c_sky->sunDist);
                break;

            default:
                std::cout << "MoveType not found" << std::endl;
                break;
            }
        }
    }
}

void s_Model::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    std::vector<uint32_t> entities = em->getEntitySet("model");
    const c_Model* c_model;
    const c_ModelMatrix* c_mm;

    const c_Engine* c_eng = (c_Engine*)em->getSComponent("engine");
    const c_Camera* c_cam = (c_Camera*)em->getSComponent("camera");
    const c_Lights* c_lights = (c_Lights*)em->getSComponent("lights");
    if (!c_eng || !c_cam || !c_lights) std::cout << "Component not found" << std::endl;
    
    uint8_t* dest;
    int i;
    
    for (uint32_t eId : entities)
    {
        c_model = (c_Model*)em->getComponent("model", eId);

        switch (c_model->model_type)
        {
        case ModelType::normal:
            switch (c_model->ubo_type)
            {
            case UboType::mvp:
                c_mm = (c_ModelMatrix*)em->getComponent("mm", eId);
                if (!c_mm) std::cout << "Component not found" << std::endl;
                for (i = 0; i < ((c_Model_normal*)c_model)->model->vsDynUBO.numDynUBOs; i++)
                {
                    dest = ((c_Model_normal*)c_model)->model->vsDynUBO.getUBOptr(i);
                    memcpy(dest + 0 * size.mat4, &c_mm->modelMatrix, sizeof(c_mm->modelMatrix));
                    memcpy(dest + 1 * size.mat4, &c_cam->view, sizeof(c_cam->view));
                    memcpy(dest + 2 * size.mat4, &c_cam->proj, sizeof(c_cam->proj));
                }
                break;

            case UboType::noData:
                break;

            default:
                std::cout << "Wrong UBO type" << std::endl;
            }
            break;

        case ModelType::planet:
            ((c_Model_planet*)c_model)->planet->updateState(c_cam->camPos, c_cam->view, c_cam->proj, c_lights->lights, c_eng->time, 100);   // <<< groundHeight
            ((c_Model_planet*)c_model)->planet->toLastDraw();
            break;

        default:
            std::cout << "Wrong model type" << std::endl;
            break;
        }
    }
}

void s_Template::update(float timeStep)
{
    #ifdef DEBUG_SYSTEM
        std::cout << typeid(this).name() << "::" << __func__ << std::endl;
    #endif

    std::vector<uint32_t> entities = em->getEntitySet("xxx");
    c_ModelMatrix* c_xxx;
    c_Move* c_mov;

    for (uint32_t eId : entities)
    {
        c_xxx = (c_ModelMatrix*)em->getComponent("xxx", eId);
        c_mov = (c_Move*)em->getComponent("move", eId);

    }
}

