
#include "physics.hpp"

#include "entities.hpp"
#include "terrain.hpp"
#include "common.hpp"


EntityFactory::EntityFactory(Renderer& renderer) 
	: MainEntityFactory(), renderer(renderer) { };

std::vector<Component*> EntityFactory::createLightingPass(const c_Lights* c_lights)
{
	std::vector<float> v_quad;	// [4 * 5]
	std::vector<uint16_t> i_quad;
	getScreenQuad(v_quad, i_quad, 1.f, 0.f);	// <<< The parameter zValue doesn't represent heigth (otherwise, this value should serve for hiding one plane behind another).

	VerticesLoader vertexData(vt_32.vertexSize, v_quad.data(), 4, i_quad);
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_lightingPass"], shaderLoaders["f_lightingPass"] };
	std::vector<TextureLoader> usedTextures{ };

	ModelDataInfo modelInfo;
	modelInfo.name = "lightingPass";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_32;
	modelInfo.verticesLoader = &vertexData;
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 1;		// <<< ModelSet doesn't work if there is no VS descriptor set
	modelInfo.maxDescriptorsCount_fs = 1;
	modelInfo.UBOsize_vs = 1;
	modelInfo.UBOsize_fs = size.vec4 + c_lights->lights.bytesSize;			// (camPos + numLights),  n * LightPosDir (2*vec4),  n * LightProps (6*vec4)
	modelInfo.globalUBO_vs;
	modelInfo.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 1;
	modelInfo.subpassIndex = 0;
	
	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> {
		new c_Model_normal(model, UboType::lightPass)
	};
}

std::vector<Component*> EntityFactory::createPostprocessingPass(const c_Lights* c_lights)
{
	std::vector<float> v_quad;	// [4 * 5]
	std::vector<uint16_t> i_quad;
	getScreenQuad(v_quad, i_quad, 1.f, 0.f);	// <<< The parameter zValue doesn't represent heigth (otherwise, this value should serve for hiding one plane behind another).

	VerticesLoader vertexData(vt_32.vertexSize, v_quad.data(), 4, i_quad);
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_postprocessing"], shaderLoaders["f_postprocessing"] };
	std::vector<TextureLoader> usedTextures{ };

	ModelDataInfo modelInfo;
	modelInfo.name = "postprocessingPass";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_32;
	modelInfo.verticesLoader = &vertexData;
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 1;		// <<< ModelSet doesn't work if there is no VS descriptor set
	modelInfo.maxDescriptorsCount_fs = 1;
	modelInfo.UBOsize_vs = 1;
	modelInfo.UBOsize_fs = 1;
	modelInfo.globalUBO_vs;
	modelInfo.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 3;
	modelInfo.subpassIndex = 0;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> {
		new c_Model_normal(model, UboType::noData)
	};
}

std::vector<Component*> EntityFactory::createNoPP()
{
	std::vector<float> v_quad;	// [4 * 5]
	std::vector<uint16_t> i_quad;
	getScreenQuad(v_quad, i_quad, 1.f, 0.5);	// <<< The parameter zValue doesn't represent heigth (otherwise, this value should serve for hiding one plane behind another).

	VerticesLoader vertexData(vt_32.vertexSize, v_quad.data(), 4, i_quad);
	//std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	//std::vector<TextureLoader> textureSet{ textures };

	ModelDataInfo modelInfo;
	modelInfo.name = "noPP";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_32;
	modelInfo.verticesLoader = &vertexData;
	//modelInfo.shadersInfo = &shaders;
	//modelInfo.texturesInfo = &textureSet;
	modelInfo.maxDescriptorsCount_vs = 1;		// <<< ModelSet doesn't work if there is no VS descriptor set
	modelInfo.UBOsize_vs = 1;
	modelInfo.UBOsize_fs = 0;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 1;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::noData) 
	};
}

std::vector<Component*> EntityFactory::createAtmosphere(const c_Lights* c_lights)
{
	std::vector<float> v_quad;	// [4 * 5]
	std::vector<uint16_t> i_quad;
	getScreenQuad(v_quad, i_quad, 1.f, 0);	// <<< The parameter zValue doesn't represent heigth (otherwise, this value should serve for hiding one plane behind another).

	OpticalDepthTable optDepth(10, 1400, 2450, 30, pi / 20, 10);	// numOptDepthPoints, planetRadius, atmosphereRadius, heightStep, angleStep, densityFallOff
	TextureLoader texOD(optDepth.table.data(), optDepth.angleSteps, optDepth.heightSteps, "optDepth", VK_FORMAT_R32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	DensityVector density(1400, 2450, 30, 10);						// planetRadius, atmosphereRadius, heightStep, densityFallOff
	TextureLoader texDV(density.table.data(), 1, density.heightSteps, "density", VK_FORMAT_R32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	VerticesLoader vertexData(vt_32.vertexSize, v_quad.data(), 4, i_quad);
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_atmosphere"], shaderLoaders["f_atmosphere"] };
	std::vector<TextureLoader> usedTextures{ texOD, texDV };

	ModelDataInfo modelInfo;
	modelInfo.name = "atmosphere";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_32;
	modelInfo.verticesLoader = &vertexData;
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 1;		// <<< ModelSet doesn't work if there is no VS descriptor set
	modelInfo.maxDescriptorsCount_fs = 1;
	modelInfo.UBOsize_vs = 2 * size.mat4 + 8 * size.vec4;
	modelInfo.UBOsize_fs = 1;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 3;
	modelInfo.subpassIndex = 0;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> {
		new c_Model_normal(model, UboType::atmosphere)
	};
	/*
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

	ModelDataInfo modelInfo;
	modelInfo.name = "atmosphere";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_32;
	modelInfo.verticesLoader = &vertexData;
	modelInfo.shadersInfo = &shaders;
	modelInfo.texturesInfo = &textureSet;
	modelInfo.maxDescriptorsCount_vs = 1;		// <<< ModelSet doesn't work if there is no VS descriptor set
	modelInfo.UBOsize_vs = 2 * size.mat4 + 8 * size.vec4;
	modelInfo.UBOsize_fs = 0;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 1;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::atmosphere) 
	};
	*/
}

std::vector<Component*> EntityFactory::createReticule()
{
	return std::vector<Component*>{};
}

std::vector<Component*> EntityFactory::createSkyBox()
{
	//VerticesLoader vertexData(vt_32.vertexSize, v_cube.data(), 14, i_inCube);
	VerticesLoader vertexData(vt_32.vertexSize, v_skybox.data(), 6 * 4, i_skybox);
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_skybox"], shaderLoaders["f_skybox"]};
	std::vector<TextureLoader> usedTextures{ texInfos["sb_front"], texInfos["sb_back"], texInfos["sb_up"], texInfos["sb_down"], texInfos["sb_right"], texInfos["sb_left"] };
	
	ModelDataInfo modelInfo;
	modelInfo.name = "skyBox";
	modelInfo.activeInstances = 0;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_32;
	modelInfo.verticesLoader = &vertexData;
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 1;
	modelInfo.maxDescriptorsCount_fs;
	modelInfo.UBOsize_vs = 2 * size.mat4;	// M, NM
	modelInfo.UBOsize_fs;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 2;
	modelInfo.subpassIndex = 0;
	modelInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	
	modelIter model = renderer.newModel(modelInfo);
	
	return std::vector<Component*> {
		new c_Model_normal(model, UboType::mm_nm),
		new c_ModelParams(),
		new c_Move(skyOrbit)						// 1 day ≈ 30 min
	};
}

std::vector<Component*> EntityFactory::createSun()
{
	VerticesLoader vertexData(vt_32.vertexSize, v_YZquad.data(), 4, i_quad);
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_sun"], shaderLoaders["f_sun"] };
	std::vector<TextureLoader> usedTextures{ texInfos["sun"] };

	ModelDataInfo modelInfo;
	modelInfo.name = "sun";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_32;
	modelInfo.verticesLoader = &vertexData;
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 1;
	modelInfo.maxDescriptorsCount_fs;
	modelInfo.UBOsize_vs = 2 * size.mat4;	// M, NM
	modelInfo.UBOsize_fs;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = true;
	modelInfo.renderPassIndex = 2;
	modelInfo.subpassIndex = 0;
	modelInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::mm_nm),
		new c_ModelParams(glm::vec3(7, 7, 7)),
		new c_Move(sunOrbit)						// 1 year ≈ 6 hours
	};
}

std::vector<Component*> EntityFactory::createGrid()
{
	float gridStep = 50;

	std::vector<float> v_grid;
	std::vector<uint16_t> i_grid;
	size_t numVertex = getGrid(v_grid, i_grid, gridStep, 50, 10, glm::vec3(0.5, 0.5, 0.5));

	VerticesLoader vertexData(vt_33.vertexSize, v_grid.data(), numVertex, i_grid);
	//std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	//std::vector<TextureLoader> textureSet{ textures };

	ModelDataInfo modelInfo;
	modelInfo.name = "grid";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	modelInfo.vertexType = vt_33;
	modelInfo.verticesLoader = &vertexData;
	//modelInfo.shadersInfo = &shaders;
	//modelInfo.texturesInfo = &textureSet;
	modelInfo.maxDescriptorsCount_vs = 1;
	modelInfo.UBOsize_vs = 3 * size.mat4;	// M, V, P
	modelInfo.UBOsize_fs = 0;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::mvp),
		new c_ModelParams(),
		new c_Move(followCamXY, gridStep) 
	};
}

std::vector<Component*> EntityFactory::createAxes()
{
	std::vector<float> v_axis;
	std::vector<uint16_t> i_axis;
	size_t numVertex = getAxis(v_axis, i_axis, 5000, 0.9);		// getAxis(), getLongAxis()

	VerticesLoader vertexData(vt_33.vertexSize, v_axis.data(), numVertex, i_axis);
	//std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	//std::vector<TextureLoader> textureSet{ textures };

	ModelDataInfo modelInfo;
	modelInfo.name = "axis";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	modelInfo.vertexType = vt_33;
	modelInfo.verticesLoader = &vertexData;
	//modelInfo.shadersInfo = &shaders;
	//modelInfo.texturesInfo = &textureSet;
	modelInfo.maxDescriptorsCount_vs = 1;
	modelInfo.UBOsize_vs = 3 * size.mat4;	// M, V, P
	modelInfo.UBOsize_fs = 0;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> {
		new c_Model_normal(model, UboType::mvp),
			new c_ModelParams()
	};
	/*
	std::vector<float> v_axis;
	std::vector<uint16_t> i_axis;
	size_t numVertex = getAxis(v_axis, i_axis, 5000, 0.9);		// getAxis(), getLongAxis()

	VerticesLoader vertexData(vt_33.vertexSize, v_axis.data(), numVertex, i_axis);
	std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	std::vector<TextureLoader> textureSet{ textures };

	ModelDataInfo modelInfo;
	modelInfo.name = "axis";
	modelInfo.layer = 1;
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	modelInfo.vertexType = vt_33;
	modelInfo.verticesLoader = &vertexData;
	modelInfo.shadersInfo = &shaders;
	modelInfo.texturesInfo = &textureSet;
	modelInfo.maxDescriptorsCount_vs = 1;
	modelInfo.UBOsize_vs = 3 * size.mat4;	// M, V, P
	modelInfo.UBOsize_fs = 0;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::mvp),
		new c_ModelParams()
	};
	*/
}

std::vector<Component*> EntityFactory::createPoints()
{
	Icosahedron icos(400.f);	// Just created for calling destructor, which applies a multiplier.

	VerticesLoader vertexData(vt_33.vertexSize, Icosahedron::icos.data(), Icosahedron::icos.size() / 6, noIndices);
	//std::vector<ShaderLoader> shaders{ Vshader, Fshader };
	//std::vector<TextureLoader> textureSet{ textures };

	ModelDataInfo modelInfo;
	modelInfo.name = "points";
	modelInfo.activeInstances = 1;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	modelInfo.vertexType = vt_33;
	modelInfo.verticesLoader = &vertexData;
	//modelInfo.shadersInfo = &shaders;
	//modelInfo.texturesInfo = &textureSet;
	modelInfo.maxDescriptorsCount_vs = 1;
	modelInfo.UBOsize_vs = 3 * size.mat4;	// M, V, P
	modelInfo.UBOsize_fs = 0;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*> { 
		new c_Model_normal(model, UboType::mvp),
		new c_ModelParams() 
	};
}

std::vector<Component*> EntityFactory::createSphere()
{
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_seaPlanet"], shaderLoaders["f_seaPlanet"] };
	std::vector<TextureLoader> usedTextures{
		texInfos["sea_n"],
		texInfos["sea_h"],
		texInfos["sea_foam_a"],
		texInfos["sb_space1"],
		
		texInfos["sb_front"],
		texInfos["sb_back"],
		texInfos["sb_up"],
		texInfos["sb_down"],
		texInfos["sb_right"],
		texInfos["sb_left"]
	};
	
	Sphere* seaSphere = new Sphere(&renderer, glm::ivec2(2, 0), 100, 21, 7, 2, 1.f, 2000, { 0.f, 0.f, 0.f }, true);
	seaSphere->addResources(usedShaders, usedTextures);

	return std::vector<Component*>{ 
		new c_Model_planet(seaSphere) 
	};
}

std::vector<Component*> EntityFactory::createPlanet()
{
	// Create noise generator:

	std::shared_ptr<Noiser> continentalness = std::make_shared<FractalNoise_SplinePts>(		// Range [-1, 1]
		FastNoiseLite::NoiseType_Perlin,	// Noise type
		4, 4.f, 0.3f,						// Octaves, Lacunarity (for frequency), Persistence (for amplitude)
		0.1,								// Scale
		4952,								// Seed
		std::vector<std::array<float, 2>>{ {-1, -0.5}, { -0.1, -0.1 }, { 0.1, 0.1 }, { 1, 1 } });

	std::shared_ptr<Noiser> erosion = std::make_shared<FractalNoise_SplinePts>(				// Rage [0, 1]
		FastNoiseLite::NoiseType_Perlin,
		2, 5.f, 0.3f,
		0.1,
		4953,
		std::vector<std::array<float, 2>>{ {-1, 1}, { 0, 0.3 }, { 1, 0 } });

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
	const std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_planetChunk"], shaderLoaders["f_planetChunk"] };
	const std::vector<TextureLoader> usedTextures{ 
		// Plants
		texInfos["grassDry_a"],
		texInfos["grassDry_n"],
		texInfos["grassDry_s"],
		texInfos["grassDry_r"],
		texInfos["grassDry_h"],
		// Rocks
		texInfos["rocky_a"],
		texInfos["rocky_n"],
		texInfos["rocky_s"],
		texInfos["rocky_r"],
		texInfos["rocky_h"],
		// Snow
		texInfos["snow_a"],
		texInfos["snow_n"],
		texInfos["snow_s"],
		texInfos["snow_r"],
		texInfos["snow_h"],
		texInfos["snow2_a"],
		texInfos["snow2_n"],
		texInfos["snow2_s"],
		texInfos["snow_r"],		// repeated
		texInfos["snow_h"],		// repeated
		// Soils
		texInfos["sandDunes_a"],
		texInfos["sandDunes_n"],
		texInfos["sandDunes_s"],
		texInfos["sandDunes_r"],
		texInfos["sandDunes_h"],
		texInfos["sandWavy_a"],
		texInfos["sandWavy_n"],
		texInfos["sandWavy_s"],
		texInfos["sandWavy_r"],
		texInfos["sandWavy_h"],
		texInfos["squares"],
		// Water
		texInfos["sea_n"],
		texInfos["sea_h"],
		texInfos["sea_foam_a"]
	};

	Planet* planet = new Planet(&renderer, glm::ivec2(0, 0), multiNoise, 100, 29, 7, 2, 1.2f, 2000, {0.f, 0.f, 0.f}, false);
	planet->addResources(usedShaders, usedTextures);

	return std::vector<Component*>{
		new c_Model_planet(planet)
	};
}

std::vector<Component*> EntityFactory::createGrass(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights)
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
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_grass"], shaderLoaders["f_grass"]};
	std::vector<TextureLoader> usedTextures{ texInfos["grass"] };

	ModelDataInfo modelInfo;
	modelInfo.name = "grass";
	modelInfo.activeInstances = 0;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_332;			// <<< vt_332 is required when loading data from file
	modelInfo.verticesLoader = &vertexData["grass"];
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 6000;
	modelInfo.maxDescriptorsCount_fs;
	modelInfo.UBOsize_vs = 2 * size.mat4;	// M, MN
	modelInfo.UBOsize_fs;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;
	modelInfo.cullMode = VK_CULL_MODE_NONE;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*>{
		new c_Model_normal(model, UboType::mm_nm),
		new c_ModelParams(),
		new c_Distributor(6, 6, zAxisRandom, 2, true, 0, grass_callback, noiseSet)
	};
}

bool grass_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers)
{
	float height = glm::distance(pos, glm::vec3(0, 0, 0));
	if (groundSlope > 0.1 ||
		height < 2010 ||
		height > 2100)
		return false;

	return true;
}

std::vector<Component*> EntityFactory::createPlant(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights)
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
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_grass"], shaderLoaders["f_grass"]};
	std::vector<TextureLoader> usedTextures{ texInfos["plant"]};

	ModelDataInfo modelInfo;
	modelInfo.name = "plant";
	modelInfo.activeInstances = 0;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_332;			// <<< vt_332 is required when loading data from file
	modelInfo.verticesLoader = &vertexData["plant"];
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 500;
	modelInfo.maxDescriptorsCount_fs;
	modelInfo.UBOsize_vs = 2 * size.mat4;	// M, MN
	modelInfo.UBOsize_fs;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;
	modelInfo.cullMode = VK_CULL_MODE_NONE;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*>{
		new c_Model_normal(model, UboType::mm_nm),
		new c_ModelParams(),
		new c_Distributor(6, 6, zAxisRandom, 2, false, 0, plant_callback, noiseSet)
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

std::vector<Component*> EntityFactory::createRock(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights)
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
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_stone"], shaderLoaders["f_stone"] };
	std::vector<TextureLoader> usedTextures{ texInfos["stone_a"], texInfos["stone_s"], texInfos["stone_r"], texInfos["stone_n"] };

	ModelDataInfo modelInfo;
	modelInfo.name = "rock";
	modelInfo.activeInstances = 0;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_332;				// <<< vt_332 is required when loading data from file
	modelInfo.verticesLoader = &vertexData["stone"];
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 500;
	modelInfo.maxDescriptorsCount_fs;
	modelInfo.UBOsize_vs = 2 * size.mat4;	// M, MN
	modelInfo.UBOsize_fs;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;
	modelInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*>{
		new c_Model_normal(model, UboType::mm_nm),
			new c_ModelParams(),
			new c_Distributor(6, 6, allAxesRandom, 5, false, 0, stone_callback, noiseSet)
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

std::vector<std::vector<Component*>> EntityFactory::createTree(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights)
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
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_trunk"], shaderLoaders["f_trunk"] };
	std::vector<TextureLoader> usedTextures{ texInfos["bark_a"] };

	ModelDataInfo modelInfo;
	modelInfo.name = "tree_trunk";
	modelInfo.activeInstances = 0;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_332;				// <<< vt_332 is required when loading data from file
	modelInfo.verticesLoader = &vertexData["trunk"];
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 500;
	modelInfo.maxDescriptorsCount_fs;
	modelInfo.UBOsize_vs = 2 * size.mat4;	// M, MN
	modelInfo.UBOsize_fs;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;
	modelInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	modelIter model = renderer.newModel(modelInfo);

	entities.push_back(std::vector<Component*>{ 
		new c_Model_normal(model, UboType::mm_nm),
		new c_ModelParams(),
		new c_Distributor(6, 6, zAxisRandom, 2, false, 0, tree_callback, noiseSet)
	});
	
	// Branches:

	//VerticesLoader vertexData2(vertexDir + "tree/branches.obj");
	std::vector<ShaderLoader> usedShaders2{ shaderLoaders["v_branch"], shaderLoaders["f_branch"]};
	std::vector<TextureLoader> usedTextureSet2{ texInfos["branch_a"]};

	modelInfo;
	modelInfo.name = "tree_branches";
	modelInfo.activeInstances = 0;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_332;				// <<< vt_332 is required when loading data from file
	modelInfo.verticesLoader = &vertexData["branches"];
	modelInfo.shadersInfo = &usedShaders2;
	modelInfo.texturesInfo = &usedTextureSet2;
	modelInfo.maxDescriptorsCount_vs = 500;
	modelInfo.maxDescriptorsCount_fs;
	modelInfo.UBOsize_vs = 4 * size.mat4 + size.vec4;	// M, V, P, MN, camPos_time, n * LightPosDir (2*vec4)
	modelInfo.UBOsize_fs;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;
	modelInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	modelIter model2 = renderer.newModel(modelInfo);

	entities.push_back(std::vector<Component*>{ 
		new c_Model_normal(model2, UboType::mm_nm),
		new c_ModelParams(),
		new c_Distributor(6, 6, zAxisRandom, 2, false, 0, tree_callback, noiseSet)
	});
	
	return entities;
}

std::vector<Component*> EntityFactory::createTreeBillboard(std::map<std::string, VerticesLoader>& vertexData, const c_Lights* c_lights)
{
	const LightSet* lights;
	if (c_lights) lights = &c_lights->lights;
	else {
		std::cout << "No c_Light component found" << std::endl;
		return std::vector<Component*>();
	}

	std::vector<std::shared_ptr<Noiser>> noiseSet;
	noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 1, 1115));
	noiseSet.push_back(std::make_shared<SimpleNoise>(FastNoiseLite::NoiseType_Value, 0.0001, 1116));

	//VerticesLoader vertexData(vertexDir + "grass.obj");
	std::vector<ShaderLoader> usedShaders{ shaderLoaders["v_treeBB"], shaderLoaders["f_treeBB"] };
	std::vector<TextureLoader> usedTextures{ texInfos["treeBB_a"] };

	ModelDataInfo modelInfo;
	modelInfo.name = "treeBB";
	modelInfo.activeInstances = 0;
	modelInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	modelInfo.vertexType = vt_332;			// <<< vt_332 is required when loading data from file
	modelInfo.verticesLoader = &vertexData["treeBB"];
	modelInfo.shadersInfo = &usedShaders;
	modelInfo.texturesInfo = &usedTextures;
	modelInfo.maxDescriptorsCount_vs = 500;
	modelInfo.maxDescriptorsCount_fs;
	modelInfo.UBOsize_vs = 2 * size.mat4;	// M, MN
	modelInfo.UBOsize_fs;
	modelInfo.globalUBO_vs = &renderer.globalUBO_vs;
	modelInfo.globalUBO_fs = &renderer.globalUBO_fs;
	modelInfo.transparency = false;
	modelInfo.renderPassIndex = 0;
	modelInfo.cullMode = VK_CULL_MODE_NONE;

	modelIter model = renderer.newModel(modelInfo);

	return std::vector<Component*>{
		new c_Model_normal(model, UboType::mm_nm),
			new c_ModelParams(),
			new c_Distributor(5, 4, zAxisRandom, 2, false, 1, tree_callback, noiseSet)
	};
}

bool tree_callback(const glm::vec3& pos, float groundSlope, const std::vector<std::shared_ptr<Noiser>>& noisers)
{
	float height = glm::distance(pos, glm::vec3(0, 0, 0));
	if (groundSlope > 0.22 ||
		height < 2010 || 
		height > 2100 ||
		noisers[0]->getNoise(pos.x, pos.y, pos.z) < 0 ||
		noisers[1]->getNoise(pos.x, pos.y, pos.z) < 0.90)
		return false;

	return true;
}