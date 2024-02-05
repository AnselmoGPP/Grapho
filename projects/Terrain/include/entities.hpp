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

	std::vector<Component*> createLightingPass(std::map<std::string, ShaderLoader>& shaders, std::map<std::string, TextureLoader>& texInfos, const c_Lights* c_lights);

	std::vector<Component*> createNoPP(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createAtmosphere(ShaderLoader Vshader, ShaderLoader Fshader);
	std::vector<Component*> createReticule(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createPoints(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createAxes(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createGrid(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createSun(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures);
	std::vector<Component*> createSkyBox(ShaderLoader Vshader, ShaderLoader Fshader, std::vector<TextureLoader>& textures);
	std::vector<Component*> createSphere(std::map<std::string, ShaderLoader>& shaders, std::map<std::string, TextureLoader>& texInfos);
	std::vector<Component*> createPlanet(std::map<std::string, ShaderLoader>& shaders, std::map<std::string, TextureLoader>& texInfos);
	std::vector<Component*> createPlant(std::map<std::string, ShaderLoader>& shaders, std::map<std::string, TextureLoader>& texInfos, std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
	std::vector<Component*> createGrass(std::map<std::string, ShaderLoader>& shaders, std::map<std::string, TextureLoader>& texInfos, std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
	std::vector<Component*> createRock(std::map<std::string, ShaderLoader>& shaders, std::map<std::string, TextureLoader>& texInfos, std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
	std::vector<Component*> createTreeBillboard(std::map<std::string, ShaderLoader>& shaders, std::map<std::string, TextureLoader>& texInfos, std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
	std::vector<std::vector<Component*>> createTree(std::map<std::string, ShaderLoader>& shaders, std::map<std::string, TextureLoader>& texInfos, std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
};

bool grass_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);
bool plant_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);
bool tree_callback (const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);
bool stone_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);

#endif
