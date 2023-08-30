#include "components.hpp"

c_Engine::c_Engine(Renderer& renderer)
	: Component("engine"), 
	r(renderer),
	io(renderer.getWindowManager()),
	width(renderer.getScreenSize().x),
	height(renderer.getScreenSize().y)
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

	// Speed
	std::cout << "keysSpeed = " << keysSpeed << std::endl;
	std::cout << "mouseSpeed = " << mouseSpeed << std::endl;
	std::cout << "scrollSpeed = " << scrollSpeed << std::endl;

	std::cout << "----------" << std::endl;
}

void c_Camera::printInfo() const
{
	std::cout << "camPos = "; printVec(camPos);
	std::cout << "front = "; printVec(front);
	std::cout << "right = "; printVec(right);
	std::cout << "camUp = "; printVec(camUp);
	std::cout << "worldUp = "; printVec(worldUp);

	std::cout << "fov = " << fov << " (" << minFov << ", " << maxFov << ')' << std::endl;
	std::cout << "View planes = (" << nearViewPlane << ", " << farViewPlane << std::endl;

	std::cout << "----------" << std::endl;
}

void c_Position::printInfo() const
{
	std::cout << "pos = "; printVec(pos);
	std::cout << "followCam = " << followCam << std::endl;

	std::cout << "----------" << std::endl;
}

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