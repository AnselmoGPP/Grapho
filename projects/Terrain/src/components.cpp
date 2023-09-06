#include "components.hpp"

c_Engine::c_Engine(Renderer& renderer)
	: Component("engine"), 
	r(renderer),
	io(renderer.getWindowManager()),
	width(renderer.getScreenSize().x),
	height(renderer.getScreenSize().y),
	time(0)
{ };

void c_Engine::printInfo() const
{
	std::cout << "width = " << width << std::endl;
	std::cout << "height = " << height << std::endl;

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

c_Camera::c_Camera(unsigned mode) : Component("camera") 
{ 
	switch (mode)
	{
	case 1:			// sphere
		keysSpeed = 50;
		mouseSpeed = 0.002;
		scrollSpeed = 0.1;
		maxPitch = 1.3;
		radius = 1000;
		minRadius = 100;
		maxRadius = 2000;
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

void c_Scale::printInfo() const
{
	std::cout << "scale: "; printVec(scale);
}

void c_Rotation::printInfo() const
{
	std::cout << "rotQuat: "; printVec(rotQuat);
}

c_Move::c_Move(MoveType moveType, float jumpStep, glm::vec3 position)
	: Component("move"), pos(position), moveType(moveType), jumpStep(jumpStep), speed(0), dist(0) { };

c_Move::c_Move(MoveType moveType, float dist, float speed, glm::vec3 position)
	: Component("move"), pos(position), moveType(moveType), jumpStep(0), speed(speed), dist(dist) { };

void c_Move::printInfo() const
{
	std::cout << "pos = "; printVec(pos);
	std::cout << "followCam = " << followCam << std::endl;

	std::cout << "----------" << std::endl;
}

c_ModelMatrix::c_ModelMatrix()
	: Component("mm"), modelMatrix(modelMatrix2(scaling, rotQuat, translation)) { };

c_ModelMatrix::c_ModelMatrix(glm::vec4 rotQuat)
	: Component("mm"), rotQuat(rotQuat), modelMatrix(modelMatrix2(scaling, rotQuat, translation)) { };

c_ModelMatrix::c_ModelMatrix(float scaleFactor)
	: Component("mm"), 
	scaling(scaleFactor, scaleFactor, scaleFactor), 
	modelMatrix(modelMatrix2(scaling, rotQuat, translation)) { };

void c_ModelMatrix::printInfo() const
{
	std::cout << "Scaling = "; printVec(scaling);
	std::cout << "Rotation = "; printVec(rotQuat);
	std::cout << "Translation = "; printVec(translation);

	std::cout << "----------" << std::endl;
}

c_Lights::c_Lights(unsigned count) : Component("lights"), lights(count) { }

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
/*
c_Sun::c_Sun(float initialDayTime, float speed, float angularWidth, float distance, unsigned mode)
	: Component("sun"), mode(mode == 1 ? 1 : 2), angularWidth(angularWidth), speed(speed), distance(distance), initialDayTime(initialDayTime), dayTime(initialDayTime) { }

void c_Sun::printInfo() const
{
	std::cout << "mode: " << mode << std::endl;
	std::cout << "angularWidth: " << angularWidth << std::endl;
	std::cout << "speed: " << speed << std::endl;
	std::cout << "distance: " << distance << std::endl;
	std::cout << "initialDayTime: " << initialDayTime << std::endl;
	std::cout << "dayTime: " << dayTime << std::endl;
}
*/