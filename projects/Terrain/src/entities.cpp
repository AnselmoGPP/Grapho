#include "entities.hpp"
#include "terrain.hpp"


EntityFactory::EntityFactory(Renderer& renderer) : MainEntityFactory(), renderer(renderer) { };

std::vector<Component*> EntityFactory::createNoPP(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures)
{
	std::vector<float> v_quad;	// [4 * 5]
	std::vector<uint16_t> i_quad;
	getScreenQuad(v_quad, i_quad, 1.f, 0.5);	// <<< The parameter zValue doesn't represent heigth (otherwise, this value should serve for hiding one plane behind another).

	VerticesLoader vertexData(vt_32.vertexSize, v_quad.data(), 4, i_quad);
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"noPP",
		2, 1, primitiveTopology::triangle, vt_32,		// For post-processing, we select an out-of-range layer so this model is not processed in the first pass (layers are only used in first pass).
		vertexData, shaders, textureSet,
		1, 1,											// <<< ModelSet doesn't work if there is no dynUBO_vs
		0,
		false,
		1);

	return std::vector<Component*> { new c_Model_normal(model, UboType::noData) };
}

std::vector<Component*> EntityFactory::createAtmosphere(ShaderLoader Vshader, ShaderLoader Fshader)
{
	std::vector<float> v_quad;	// [4 * 5]
	std::vector<uint16_t> i_quad;
	getScreenQuad(v_quad, i_quad, 1.f, 0.5);

	OpticalDepthTable optDepth(10, 1400, 2450, 30, pi / 20, 10);	// numOptDepthPoints, planetRadius, atmosphereRadius, heightStep, angleStep, densityFallOff
	TextureLoader texOD(optDepth.table.data(), optDepth.angleSteps, optDepth.heightSteps, "optDepth", VK_FORMAT_R32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	DensityVector density(1400, 2450, 30, 10);						// planetRadius, atmosphereRadius, heightStep, densityFallOff
	TextureLoader texDV(density.table.data(), 1, density.heightSteps, "density", VK_FORMAT_R32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	VerticesLoader vertexData(vt_32.vertexSize, v_quad.data(), 4, i_quad);
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ texOD, texDV };

	modelIter model = renderer.newModel(
		"atmosphere",
		2, 1, primitiveTopology::triangle, vt_32,
		vertexData, shaders, textureSet,
		1, 2 * size.mat4 + 8 * size.vec4,
		0,
		false,
		1);

	return std::vector<Component*> { new c_Model_normal(model, UboType::atmosphere) };
}

std::vector<Component*> EntityFactory::createReticule(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures)
{
	return std::vector<Component*>{};
}

std::vector<Component*> EntityFactory::createSkyBox(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures)
{
	VerticesLoader vertexData(vt_32.vertexSize, v_cube.data(), 14, i_inCube);
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"skyBox",
		0, 1, primitiveTopology::triangle, vt_32,
		vertexData, shaders, textureSet,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);

	//return std::vector<Component*> { new c_Model(model), new c_ModelMatrix(100), new c_Move(followCam) };
	return std::vector<Component*> {
		new c_Model_normal(model, UboType::mvp),
		new c_ModelMatrix(100),
		new c_Move(skyOrbit)		// 1 day ≈ 30 min
	};
}

std::vector<Component*> EntityFactory::createSun(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures)
{
	std::vector<float> v_sun;	// [4 * 5]
	std::vector<uint16_t> i_sun;
	size_t numVertex = getQuad(v_sun, i_sun, 1.f, 1.f, 0.f);		// LOOK dynamic adjustment of reticule size when window is resized

	VerticesLoader vertexData(vt_32.vertexSize, v_sun.data(), numVertex, i_sun);
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"sun",
		0, 1, primitiveTopology::triangle, vt_32,
		vertexData, shaders, textureSet,
		1, 3 * size.mat4,	// M, V, P
		0,
		true);

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::mvp),
		new c_ModelMatrix(10),
		new c_Move(sunOrbit)	// 1 year ≈ 6 hours
	};
}

std::vector<Component*> EntityFactory::createGrid(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures)
{
	float gridStep = 50;

	std::vector<float> v_grid;
	std::vector<uint16_t> i_grid;
	size_t numVertex = getGrid(v_grid, i_grid, gridStep, 50, 10, glm::vec3(0.5, 0.5, 0.5));

	VerticesLoader vertexData(vt_33.vertexSize, v_grid.data(), numVertex, i_grid);
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"grid",
		1, 1, primitiveTopology::line, vt_33,
		vertexData, shaders, textureSet,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);

	return std::vector<Component*> { new c_Model_normal(model, UboType::mvp), new c_ModelMatrix(), new c_Move(followCamXY, gridStep) };
}

std::vector<Component*> EntityFactory::createAxes(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures)
{
	std::vector<float> v_axis;
	std::vector<uint16_t> i_axis;
	size_t numVertex = getAxis(v_axis, i_axis, 5000, 0.9);		// getAxis(), getLongAxis()

	VerticesLoader vertexData(vt_33.vertexSize, v_axis.data(), numVertex, i_axis);
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"axis",
		1, 1, primitiveTopology::line, vt_33,
		vertexData, shaders, textureSet,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);

	return std::vector<Component*> { new c_Model_normal(model, UboType::mvp), new c_ModelMatrix() };
}

std::vector<Component*> EntityFactory::createPoints(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures)
{
	Icosahedron icos(400.f);	// Just created for calling destructor, which applies a multiplier.

	VerticesLoader vertexData(vt_33.vertexSize, Icosahedron::icos.data(), Icosahedron::icos.size() / 6, noIndices);
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"points",
		1, 1, primitiveTopology::point, vt_33,
		vertexData, shaders, textureSet,
		1, 3 * size.mat4,	// M, V, P
		0,
		false);

	return std::vector<Component*> { new c_Model_normal(model, UboType::mvp), new c_ModelMatrix() };
}

std::vector<Component*> EntityFactory::createSphere(ShaderLoader Vshader, ShaderLoader Fshader, std::vector<TextureLoader>& textures)
{
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };

	Sphere* seaSphere = new Sphere(&renderer, 100, 29, 8, 2, 1.f, 2010, { 0.f, 0.f, 0.f }, true);
	seaSphere->addResources(shaders, textures);

	return std::vector<Component*>{ new c_Model_planet(seaSphere) };
}

std::vector<Component*> EntityFactory::createPlanet(ShaderLoader Vshader, ShaderLoader Fshader, std::vector<TextureLoader>& textures)
{
	std::shared_ptr<Noiser> noiser_hills = std::make_shared<SingleNoise>(
		FastNoiseLite::NoiseType_Perlin,	// Noise type
		3, 6.f, 0.1f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
		3, 120,								// Scale, Multiplier
		0,									// Curve degree
		0, 0, 0,							// XYZ offsets
		4952);								// Seed

	std::vector<ShaderLoader> shaders{ Vshader, Fshader };

	Planet* planet = new Planet(&renderer, noiser_hills, 100, 29, 8, 2, 1.2f, 2000, { 0.f, 0.f, 0.f }, false);
	planet->addResources(shaders, textures);
	
	return std::vector<Component*>{ new c_Model_planet(planet) };
}

std::vector<Component*> EntityFactory::createGrass(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures, const c_Lights* c_lights)
{
	const LightSet* lights;
	if (c_lights) lights = &c_lights->lights;
	else { 
		std::cout << "No c_Light component found" << std::endl; 
		return std::vector<Component*>();
	}
	
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	GrassSystem_planet* grass = new GrassSystem_planet(renderer, 20, 6);
	grass->createGrassModel(shaders, textureSet, lights);

	return std::vector<Component*>{ new c_Model_grassPlanet(grass) };
}
