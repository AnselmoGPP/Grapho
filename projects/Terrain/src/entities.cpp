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

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::noData) 
	};
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

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::atmosphere) 
	};
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
		0 );

	//return std::vector<Component*> { new c_Model(model), new c_ModelMatrix(100), new c_Move(followCam) };
	return std::vector<Component*> {
		new c_Model_normal(model, UboType::mvp),
		new c_ModelParams(glm::vec3(100,100,100)),
		new c_Move(skyOrbit)						// 1 day ≈ 30 min
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
		new c_ModelParams(glm::vec3(10, 10, 10)),
		new c_Move(sunOrbit)						// 1 year ≈ 6 hours
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
		0 );

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::mvp),
		new c_ModelParams(),
		new c_Move(followCamXY, gridStep) 
	};
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
		0 );

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::mvp),
		new c_ModelParams()
	};
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
		0 );

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::mvp),
		new c_ModelParams() 
	};
}

std::vector<Component*> EntityFactory::createSphere(ShaderLoader Vshader, ShaderLoader Fshader, std::vector<TextureLoader>& textures)
{
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };

	Sphere* seaSphere = new Sphere(&renderer, 100, 29, 8, 2, 1.f, 2000, { 0.f, 0.f, 0.f }, true);
	seaSphere->addResources(shaders, textures);

	return std::vector<Component*>{ 
		new c_Model_planet(seaSphere) 
	};
}

std::vector<Component*> EntityFactory::createPlanet(ShaderLoader Vshader, ShaderLoader Fshader, std::vector<TextureLoader>& textures)
{
	// Create noise generator:

	std::shared_ptr<Noiser> continentalness = std::make_shared<FractalNoise_SplinePts>(		// Range [-1, 1]
		FastNoiseLite::NoiseType_Perlin,	// Noise type
		4, 4.f, 0.3f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
		0.1,								// Scale
		4952,								// Seed
		std::vector<std::array<float, 2>>{ {-1, -0.5}, {-0.1, -0.1}, { 0.1, 0.1 }, { 1, 1 } } );

	std::shared_ptr<Noiser> erosion = std::make_shared<FractalNoise_SplinePts>(				// Rage [0, 1]
		FastNoiseLite::NoiseType_Perlin,
		2, 5.f, 0.3f,
		0.1,
		4953,
		std::vector<std::array<float, 2>>{ {-1, 1}, { 0, 0.3 }, { 1, 0} } );

	std::shared_ptr<Noiser> PV = std::make_shared<FractalNoise_SplinePts>(					// Range [-1, 1]
		FastNoiseLite::NoiseType_Perlin,
		2, 2.f, 0.3f,
		0.5,
		4954,
		std::vector<std::array<float, 2>>{ {-1, 0}, { -0.3, 1 }, { 0.6, 0 }, { 1, 1 } });

	std::shared_ptr<Noiser> temperature;

	std::shared_ptr<Noiser> humidity;

	std::vector<std::shared_ptr<Noiser>> noiserSet = { continentalness, erosion, PV, temperature, humidity };
	std::shared_ptr<Noiser> multiNoise = std::make_shared<Multinoise>(noiserSet, getNoise_C_E_PV);
	
	// Create planet entity:

	std::vector<ShaderLoader> shaders{ Vshader, Fshader };

	Planet* planet = new Planet(&renderer, multiNoise, 100, 29, 8, 2, 1.2f, 2000, { 0.f, 0.f, 0.f }, false);
	planet->addResources(shaders, textures);
	
	return std::vector<Component*>{ 
		new c_Model_planet(planet) 
	};
}

std::vector<Component*> EntityFactory::createGrass(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures, VerticesLoader& vertexData, const c_Lights* c_lights)
{
	const LightSet* lights;
	if (c_lights) lights = &c_lights->lights;
	else {
		std::cout << "No c_Light component found" << std::endl;
		return std::vector<Component*>();
	}

	std::vector<std::shared_ptr<Noiser>> noiseSet;
	//noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 1, 1111));
	//noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 0.1, 1112));

	//VerticesLoader vertexData(vertexDir + "grass.obj");
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"grass",
		1, 1, primitiveTopology::triangle, vt_332,	// <<< vt_332 is required when loading data from file
		vertexData, shaders, textureSet,
		1, 4 * size.mat4 + size.vec4 + c_lights->lights.numLights * sizeof(LightPosDir),	// M, V, P, MN, camPos_time, n * LightPosDir (2*vec4)
		c_lights->lights.numLights * sizeof(LightProps),									// n * LightProps (6*vec4)
		0, 0,
		VK_CULL_MODE_NONE);

	return std::vector<Component*>{
		new c_Model_normal(model, UboType::mvpncl),
		new c_ModelParams(),
		new c_Distributor(7, 1, 1, grass_callback, noiseSet)
	};
}

bool grass_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers)
{
	float height = glm::distance(pos, glm::vec3(0, 0, 0));
	if (groundSlope > 0.08 ||
		height < 2010 ||
		height > 2100)
		return false;

	return true;
}

std::vector<Component*> EntityFactory::createPlant(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures, VerticesLoader& vertexData, const c_Lights* c_lights)
{
	const LightSet* lights;
	if (c_lights) lights = &c_lights->lights;
	else {
		std::cout << "No c_Light component found" << std::endl;
		return std::vector<Component*>();
	}

	std::vector<std::shared_ptr<Noiser>> noiseSet;
	noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 1, 1111));
	noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 0.1, 1112));

	//VerticesLoader vertexData(vertexDir + "grass.obj");
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"plant",
		1, 1, primitiveTopology::triangle, vt_332,	// <<< vt_332 is required when loading data from file
		vertexData, shaders, textureSet,
		1, 4 * size.mat4 + size.vec4 + c_lights->lights.numLights * sizeof(LightPosDir),	// M, V, P, MN, camPos_time, n * LightPosDir (2*vec4)
		c_lights->lights.numLights * sizeof(LightProps),									// n * LightProps (6*vec4)
		0, 0,
		VK_CULL_MODE_NONE);

	return std::vector<Component*>{
		new c_Model_normal(model, UboType::mvpncl),
		new c_ModelParams(),
		new c_Distributor(7, 1, 2, plant_callback, noiseSet)
	};
}

bool plant_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers)
{
	float height = glm::distance(pos, glm::vec3(0, 0, 0));
	if (groundSlope > 0.22 ||
		height < 2010 ||
		height > 2100 ||
		noisers[0]->getNoise(pos.x, pos.y, pos.z) < 0 ||
		noisers[1]->getNoise(pos.x, pos.y, pos.z) < 0.7)
		return false;

	return true;
}

std::vector<Component*> EntityFactory::createRock(ShaderLoader Vshader, ShaderLoader Fshader, std::initializer_list<TextureLoader> textures, VerticesLoader& vertexData, const c_Lights* c_lights)
{
	const LightSet* lights;
	if (c_lights) lights = &c_lights->lights;
	else {
		std::cout << "No c_Light component found" << std::endl;
		return std::vector<Component*>();
	}

	std::vector<std::shared_ptr<Noiser>> noiseSet;
	noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 1, 1113));
	noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 0.0001, 1114));

	//VerticesLoader vertexData(vertexDir + "rocks/free_rock/rock.obj");
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	modelIter model = renderer.newModel(
		"rock",
		1, 1, primitiveTopology::triangle, vt_332,	// <<< vt_332 is required when loading data from file
		vertexData, shaders, textureSet,
		1, 4 * size.mat4 + size.vec4 + c_lights->lights.numLights * sizeof(LightPosDir),	// M, V, P, MN, camPos_time, n * LightPosDir (2*vec4)
		c_lights->lights.numLights * sizeof(LightProps));									// n * LightProps (6*vec4)

	return std::vector<Component*>{
		new c_Model_normal(model, UboType::mvpncl),
			new c_ModelParams(),
			new c_Distributor(7, 2, 5, stone_callback, noiseSet)
	};
}

bool stone_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers)
{
	float height = glm::distance(pos, glm::vec3(0, 0, 0));
	if (groundSlope > 0.22 ||
		height < 2000 ||
		height > 2100 ||
		noisers[0]->getNoise(pos.x, pos.y, pos.z) < 0 ||
		noisers[1]->getNoise(pos.x, pos.y, pos.z) < 0.95)
		return false;

	return true;
}

std::vector<std::vector<Component*>> EntityFactory::createTree(std::initializer_list<ShaderLoader> trunkShaders, std::initializer_list<ShaderLoader> branchShaders, std::initializer_list<TextureLoader> tex_trunk, std::initializer_list<TextureLoader> tex_branch, VerticesLoader& vertexData_trunk, VerticesLoader& vertexData_branches, const c_Lights* c_lights)
{
	const LightSet* lights;
	if (c_lights) lights = &c_lights->lights;
	else {
		std::cout << "No c_Light component found" << std::endl;
		return std::vector<std::vector<Component*>>();
	}

	std::vector<std::shared_ptr<Noiser>> noiseSet;
	noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 1, 1115));
	noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 0.0001, 1116));

	std::vector<std::vector<Component*>> entities;

	// Trunk:
	
	//VerticesLoader vertexData(vertexDir + "tree/trunk.obj");
	std::vector<ShaderLoader> shaders = trunkShaders;
	std::vector<TextureLoader> textureSet{ tex_trunk };

	modelIter model = renderer.newModel(
		"tree_trunk",
		1, 1, primitiveTopology::triangle, vt_332,	// <<< vt_332 is required when loading data from file
		vertexData_trunk, shaders, textureSet,
		1, 4 * size.mat4 + size.vec4 + c_lights->lights.numLights * sizeof(LightPosDir),	// M, V, P, MN, camPos_time, n * LightPosDir (2*vec4)
		c_lights->lights.numLights * sizeof(LightProps));									// n * LightProps (6*vec4)

	entities.push_back(std::vector<Component*>{ 
		new c_Model_normal(model, UboType::mvpncl),
		new c_ModelParams(),
		new c_Distributor(7, 1, 2, tree_callback, noiseSet)
	});
	
	// Branches:

	//VerticesLoader vertexData2(vertexDir + "tree/branches.obj");
	std::vector<ShaderLoader> shaders2 = branchShaders;
	std::vector<TextureLoader> textureSet2{ tex_branch };

	modelIter model2 = renderer.newModel(
		"tree_branches",
		1, 1, primitiveTopology::triangle, vt_332,	// <<< vt_332 is required when loading data from file
		vertexData_branches, shaders2, textureSet2,
		1, 4 * size.mat4 + size.vec4 + c_lights->lights.numLights * sizeof(LightPosDir),	// M, V, P, MN, camPos_time, n * LightPosDir (2*vec4)
		c_lights->lights.numLights * sizeof(LightProps));									// n * LightProps (6*vec4)

	entities.push_back(std::vector<Component*>{ 
		new c_Model_normal(model2, UboType::mvpncl),
		new c_ModelParams(),
		new c_Distributor(7, 1, 2, tree_callback, noiseSet)
	});
	
	return entities;
}

bool tree_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers)
{
	float height = glm::distance(pos, glm::vec3(0, 0, 0));
	if (groundSlope > 0.22 ||
		height < 2010 || 
		height > 2100 ||
		noisers[0]->getNoise(pos.x, pos.y, pos.z) < 0 ||
		noisers[1]->getNoise(pos.x, pos.y, pos.z) < 0.95)
		return false;

	return true;
}