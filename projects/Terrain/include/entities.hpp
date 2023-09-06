#ifndef ENTITIES_HPP
#define ENTITIES_HPP

#include "Renderer.hpp"
#include "Toolkit.hpp"
#include "ECSarch.hpp"

#include "components.hpp"


class EntityFactory : public MainEntityFactory
{
	Renderer& renderer;

public:
	EntityFactory(Renderer& renderer);

	std::vector<Component*> createSeaPlanet	(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createSkyBox	(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createSun		(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createGrid		(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createAxes		(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createPoints	(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createNoPP		(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
};

#endif
