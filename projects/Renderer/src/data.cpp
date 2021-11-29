
#include "data.hpp"


/// Get the model matrix (MM). This function puts the asset in the origin and makes it rotate around z-axis.
glm::mat4 standard_MM(float time)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

	return model;
}

// Cottage config data --------------------

glm::mat4 cottage_MM(float time)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, time * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

	return model;
}


// Room config data --------------------

glm::mat4 room1_MM(float time)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -50.0f, 3.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
	return model;
}

glm::mat4 room2_MM(float time)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -80.0f, 3.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));

	return model;
}

glm::mat4 room3_MM(float time)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(30.0f, -80.0f, 3.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));

	return model;
}

glm::mat4 room4_MM(float time)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(30.0f, -50.0f, 3.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));

	return model;
}

std::vector< std::function<glm::mat4(float)> > room_MM{ room1_MM, room2_MM, room3_MM, room4_MM };

glm::mat4 room5_MM(float time)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(30.0f, -50.0f, 30.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(40.0f, 40.0f, 40.0f));

	return model;
}

// Others --------------------

std::vector<Vertex> v_floor = { 
	Vertex(glm::vec3(-100,  100, 0), glm::vec3(1, 1, 1) , glm::vec2(-10,  10)) ,
	Vertex(glm::vec3(-100, -100, 0), glm::vec3(1, 1, 1) , glm::vec2(-10, -10)),
	Vertex(glm::vec3( 100, -100, 1), glm::vec3(1, 1, 1) , glm::vec2( 10, -10)),
	Vertex(glm::vec3( 100,  100, 1), glm::vec3(1, 1, 1) , glm::vec2( 10,  10))
};

extern std::vector<uint32_t> i_floor = { 0, 1, 3,  1, 2, 3 };


