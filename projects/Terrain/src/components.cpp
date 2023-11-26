#include "components.hpp"

c_Engine::c_Engine(Renderer& renderer)
	: Component(CT::engine), r(renderer), io(r.getIOManager()), time(0), frameCount(0)
{ };

int c_Engine::getWidth() const 
{ 
	int width, height;
	io.getFramebufferSize(&width, &height);
	return width; 
}

int c_Engine::getHeight() const 
{ 
	int width, height;
	io.getFramebufferSize(&width, &height);
	return height;
}

float c_Engine::getAspectRatio() const 
{
	int width, height;
	io.getFramebufferSize(&width, &height);
	return (float)width/height;
};


void c_Engine::printInfo() const
{
	std::cout << "width = " << getWidth() << std::endl;
	std::cout << "height = " << getHeight() << std::endl;
	std::cout << "aspectRatio = " << getAspectRatio() << std::endl;

	std::cout << "----------" << std::endl;
}

void c_Input::printInfo() const
{
	// Keyboard
	std::cout << "W_press = " << W_press << std::endl;
	std::cout << "S_press = " << S_press << std::endl;
	std::cout << "A_press = " << A_press << std::endl;
	std::cout << "D_press = " << D_press << std::endl;
	std::cout << "Q_press = " << Q_press << std::endl;
	std::cout << "E_press = " << E_press << std::endl;
	std::cout << "up_press = " << up_press << std::endl;
	std::cout << "down_press = " << down_press << std::endl;
	std::cout << "left_press = " << left_press << std::endl;
	std::cout << "right_press = " << right_press << std::endl;

	// Mouse
	std::cout << "LMB_pressed = " << LMB_pressed << std::endl;
	std::cout << "RMB_pressed = " << RMB_pressed << std::endl;
	std::cout << "MMB_pressed = " << MMB_pressed << std::endl;
	std::cout << "LMB_justPressed = " << LMB_justPressed << std::endl;
	std::cout << "RMB_justPressed = " << RMB_justPressed << std::endl;
	std::cout << "MMB_justPressed = " << MMB_justPressed << std::endl;
	std::cout << "yScrollOffset = " << yScrollOffset << std::endl;
	std::cout << "xpos = " << xpos << std::endl;
	std::cout << "ypos = " << ypos << std::endl;
	std::cout << "lastX = " << lastX << std::endl;
	std::cout << "lastY = " << lastY << std::endl;
	std::cout << "W_press = " << W_press << std::endl;
	std::cout << "W_press = " << W_press << std::endl;

	std::cout << "----------" << std::endl;
}

c_Camera::c_Camera(unsigned mode) : Component(CT::camera) 
{ 
	switch (mode)
	{
	case 1:			// sphere
		keysSpeed = 50;
		mouseSpeed = 0.002;
		scrollSpeed = 0.1;
		maxPitch = 1.3;
		radius = 10; //4000;
		minRadius = 10;
		maxRadius = 5000;
		camPos = -front * radius;
		break;
	case 2:			// polar plane
		camPos = { 0, 0, 5.0 };
		keysSpeed = 2;
		mouseSpeed = 0.001;
		scrollSpeed = 0.1;
		break;
	case 3:			// free plane
		camPos = { 1450, 1450, 0 };
		keysSpeed = 50;
		mouseSpeed = 0.001;
		scrollSpeed = 0.1;
		break;
	case 4:			// first person
		camPos = { 1450, 1450, 0 };
		keysSpeed = 2;
		mouseSpeed = 0.001;
		scrollSpeed = 0.1;
		break;
	default:
		break;
	}
	
	radius = glm::length(camPos);

	// Pitch
	euler.x = asin(front.z);

	// Yaw
	if (abs(front.x) > 0.001)
	{
		euler.z = atan(front.y / front.x);
		if (front.x < 0) euler.z = pi + euler.z;
	}
	else euler.z = pi / 2;

	// Roll
	float dotP;
	if (front.z < 0.999) {
		dotP = glm::dot(right, glm::normalize(glm::cross(front, worldUp)));
		if (dotP < 0.999) euler.y = acos(dotP);
		else euler.y = 0;
		if (right.z < 0) euler.y *= -1;
	}
	else euler.y = 0;
};

void c_Camera::printInfo() const
{
	std::cout << "camPos = "; printVec(camPos);
	std::cout << "front = "; printVec(front);
	std::cout << "right = "; printVec(right);
	std::cout << "camUp = "; printVec(camUp);
	std::cout << "worldUp = "; printVec(worldUp);
	std::cout << "euler angles = "; printVec(euler);
	std::cout << "radius = " << radius << std::endl;

	std::cout << "fov = " << fov << " (" << minFov << ", " << maxFov << ')' << std::endl;
	std::cout << "View planes = (" << nearViewPlane << ", " << farViewPlane << std::endl;

	std::cout << "keysSpeed = " << keysSpeed << std::endl;
	std::cout << "spinSpeed = " << spinSpeed << std::endl;
	std::cout << "mouseSpeed = " << mouseSpeed << std::endl;
	std::cout << "scrollSpeed = " << scrollSpeed << std::endl;

	std::cout << "----------" << std::endl;
}

c_Move::c_Move(MoveType moveType, float jumpStep, glm::vec3 position)
	: Component(CT::move), pos(position), moveType(moveType), jumpStep(jumpStep) { };

c_Move::c_Move(MoveType moveType)
	: Component(CT::move), moveType(moveType) { };

void c_Move::printInfo() const
{
	std::cout << "moveType: " << moveType << std::endl;
	std::cout << "pos: "; printVec(pos);
	std::cout << "rotQuat: "; printVec(rotQuat);
	std::cout << "jumpStep: " << jumpStep << std::endl;

	std::cout << "----------" << std::endl;
}

c_ModelMatrix::c_ModelMatrix()
	: Component(CT::modelMatrix), modelMatrix(getModelMatrix()) { };

c_ModelMatrix::c_ModelMatrix(glm::vec4 rotQuat)
	: Component(CT::modelMatrix), modelMatrix(getModelMatrix(glm::vec3(1,1,1), rotQuat, glm::vec3(0, 0, 0))) { };

c_ModelMatrix::c_ModelMatrix(float scale)
	: Component(CT::modelMatrix), scale(scale, scale, scale), modelMatrix(getModelMatrix(this->scale, glm::vec4(1, 0, 0, 0), glm::vec3(0, 0, 0))) { };

c_ModelMatrix::c_ModelMatrix(float scale, glm::vec4 rotQuat)
	: Component(CT::modelMatrix), scale(scale, scale, scale), modelMatrix(getModelMatrix(this->scale, rotQuat, glm::vec3(0, 0, 0))) { };

void c_ModelMatrix::printInfo() const
{
	std::cout << "Scale = "; printVec(scale);
	//std::cout << "Rotation = "; printVec(rotQuat);
	//std::cout << "Translation = "; printVec(translation);

	std::cout << "----------" << std::endl;
}

c_Lights::c_Lights(unsigned count) : Component(CT::lights), lights(count) 
{ 
	unsigned i = 0;

	//lights.turnOff(0);

	// Sun (day & night):
	if (i < count) lights.setDirectional(i++,  glm::vec3(-1,0,0), glm::vec3(0.03, 0.03, 0.03), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));
	if (i < count) lights.setDirectional(i++, -glm::vec3(-1, 0, 0), glm::vec3(0.00, 0.00, 0.00), glm::vec3(0.01, 0.01, 0.01), glm::vec3(0.007, 0.007, 0.007));

	// Flashlight:
	if (i < count) lights.setSpot(i++, glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), glm::vec3(2, 2, 2), glm::vec3(2, 2, 2), 1, 0.09, 0.032, 0.9, 0.8);

	//lights.setPoint(1, glm::vec3(0,0,0), glm::vec3(0,0,0), glm::vec3(40, 40, 40), glm::vec3(40, 40, 40), 1, 1, 1);
	//lights.setSpot(1, glm::vec3(0,0,0), glm::vec3(0, 0,-1), glm::vec3(0, 0, 0), glm::vec3(0, 40, 40), glm::vec3(40, 40, 40), 1, 1, 1, 0.9, 0.8);
}

void c_Lights::printInfo() const
{
	for (unsigned i = 0; i < lights.numLights; i++)
	{
		std::cout << "Light " << i << ':' << std::endl;
		std::cout << "   Pos: "; printVec(lights.posDir->position);
		std::cout << "   Dir: "; printVec(lights.posDir->direction);
		std::cout << "   Type: " << lights.props->type << std::endl;
		std::cout << "   Ambient: "; printVec(lights.props->ambient);
		std::cout << "   Diffuse: "; printVec(lights.props->diffuse);
		std::cout << "   Specular: "; printVec(lights.props->specular);
		std::cout << "   Degree: "; printVec(lights.props->degree);
		std::cout << "   CutOff: "; printVec(lights.props->cutOff);	}
}

c_Sky::c_Sky(float skySpeed, float skyAngle, float sunSpeed, float sunAngle, float sunDist)
	: Component(CT::sky), 
	eclipticAngle(0.41), 
	skySpeed(skySpeed), sunSpeed(sunSpeed), 
	skyAngle_0(skyAngle), sunAngle_0(sunAngle),
	sunDist(sunDist),
	skyAngle(0), sunAngle(0), 
	sunDir(0,0,0) { }

void c_Sky::printInfo() const
{
	std::cout << "skySpeed: " << skySpeed << std::endl;
	std::cout << "skyAngle: " << skyAngle << std::endl;
	std::cout << "sunSpeed: " << sunSpeed << std::endl;
	std::cout << "sunAngle: " << sunAngle << std::endl;
}
