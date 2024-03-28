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

	std::map<std::string, ShaderLoader> shaderLoaders;
	std::map<std::string, TextureLoader> texInfos;

	std::vector<Component*> createLightingPass(const c_Lights* c_lights);
	std::vector<Component*> createPostprocessingPass(const c_Lights* c_lights);

	std::vector<Component*> createNoPP();
	std::vector<Component*> createAtmosphere(const c_Lights* c_lights);
	std::vector<Component*> createReticule();
	std::vector<Component*> createPoints();
	std::vector<Component*> createAxes();
	std::vector<Component*> createGrid();
	std::vector<Component*> createSun();
	std::vector<Component*> createSkyBox();
	std::vector<Component*> createSphere();
	std::vector<Component*> createPlanet();
	std::vector<Component*> createPlant(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
	std::vector<Component*> createGrass(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
	std::vector<Component*> createRock(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
	std::vector<Component*> createTreeBillboard(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
	std::vector<std::vector<Component*>> createTree(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights);
};

bool grass_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);
bool plant_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);
bool tree_callback (const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);
bool stone_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers);

#endif
