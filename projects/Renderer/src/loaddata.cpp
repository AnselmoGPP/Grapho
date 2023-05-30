
#include <string>
#include <unordered_map>		// For storing unique vertices from the model
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "loaddata.hpp"
#include "commons.hpp"


//VertexFromUser::VertexFromUser(size_t vertexCount, const void* vertexData, size_t indicesCount, const uint32_t* indices)
//	: vertexCount(vertexCount), vertexData(vertexData), indicesCount(indicesCount), indices(indices) { }

// VertexFromUser -------------------------------------------------------

DataFromUser_computed::DataFromUser_computed(const VertexType& vertexType, size_t vertexCount, const void* vertexData, std::vector<uint16_t>& indices, bool loadNow)
	: DataLoader(), sourceVertexCount(vertexCount), sourceVertices(vertexData), sourceIndices(indices), immediateMode(loadNow) 
{
	this->vertexType = vertexType; 
}

DataFromUser_computed::~DataFromUser_computed() { }

void DataFromUser_computed::setDestination(VertexSet& vertices, std::vector<uint16_t>& indices)
{
	this->destVertices = &vertices;
	this->destIndices = &indices;
	
	if (immediateMode) loadData();
}

void DataFromUser_computed::loadVertex() { if (!immediateMode) loadData(); }

void DataFromUser_computed::loadData()
{
	destVertices->reset(vertexType, sourceVertexCount, sourceVertices);
	*destIndices = sourceIndices;
}


// DataFromFile -------------------------------------------------------

DataFromFile::DataFromFile(const char* modelPath) : DataLoader()
{
	this->vertexType = VertexType({ 3 * sizeof(float), 3 * sizeof(float), 2 * sizeof(float) }, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT });
	copyCString(this->filePath, modelPath);
}

DataFromFile::~DataFromFile() { delete filePath; }

// (18)
void DataFromFile::loadVertex()
{
	// Load model
	tinyobj::attrib_t					 attrib;			// Holds all of the positions, normals and texture coordinates.
	std::vector<tinyobj::shape_t>		 shapes;			// Holds all of the separate objects and their faces. Each face consists of an array of vertices. Each vertex contains the indices of the position, normal and texture coordinate attributes.
	std::vector<tinyobj::material_t>	 materials;			// OBJ models can also define a material and texture per face, but we will ignore those.
	std::string							 warn, err;			// Errors and warnings that occur while loading the file.

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath))
		throw std::runtime_error(warn + err);

	// Combine all the faces in the file into a single model
	std::unordered_map<VertexPCT, uint32_t> uniqueVertices{};	// Keeps track of the unique vertices and the respective indices, avoiding duplicated vertices (not indices).

	for (const auto& shape : shapes)
		for (const auto& index : shape.mesh.indices)
		{
			// Get each vertex
			VertexPCT vertex{};	// <<< out of the loop better?

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],			// attrib.vertices is an array of floats, so we need to multiply the index by 3 and add offsets for accessing XYZ components.
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
					   attrib.texcoords[2 * index.texcoord_index + 0],	// attrib.texcoords is an array of floats, so we need to multiply the index by 3 and add offsets for accessing UV components.
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]	// Flip vertical component of texture coordinates: OBJ format assumes Y axis go up, but Vulkan has top-to-bottom orientation. 
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			// Check if we have already seen this vertex. If not, assign an index to it and save the vertex.
			if (uniqueVertices.count(vertex) == 0)			// Using a user-defined type (Vertex struct) as key in a hash table requires us to implement two functions: equality test (override operator ==) and hash calculation (implement a hash function for Vertex).
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(destVertices->size());	// Set new index for this vertex
				destVertices->push_back(&vertex);										// Save vertex
			}

			// Save the index
			destIndices->push_back(uniqueVertices[vertex]);								// Save index
		}
}


// DataFromFile2 -------------------------------------------------------

DataFromFile2::DataFromFile2(const char* modelPath)  : DataLoader()
{
	this->vertexType = VertexType({ 3 * sizeof(float), 3 * sizeof(float), 2 * sizeof(float) }, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT });
	filePath = modelPath;
}

DataFromFile2::~DataFromFile2() { }

// (18)
void DataFromFile2::loadVertex()
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);
	
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}
	
	processNode(scene->mRootNode, scene);
}

void DataFromFile2::processNode(aiNode* node, const aiScene* scene)
{
	// Process all node's meshes
	aiMesh* mesh;
	for (unsigned i = 0; i < node->mNumMeshes; i++)
	{
		mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(mesh);
		processMesh(mesh, scene);
	}

	// Repeat process in children
	for (unsigned i = 0; i < node->mNumChildren; i++)
		processNode(node->mChildren[i], scene);
}

void DataFromFile2::processMesh(aiMesh* mesh, const aiScene* scene)
{
	//std::vector<Vertex> vertices;
	//std::vector<unsigned int> indices;
	//std::vector<Textures> textures;

	// Resize vertices/index buffer
	// Find vertices/index buffer position
	// Save data there

	//destVertices->reserve(destVertices->size() + mesh->mNumVertices);
	float* vertex = new float[vertexType.vertexSize / sizeof(float)];	// [3 + 3 + 2]
	unsigned i, j;

	// Get VERTEX data (positions, normals, UVs) and store it.

	for (i = 0; i < mesh->mNumVertices; i++)
	{
		vertex[0] = mesh->mVertices[i].x;
		vertex[1] = mesh->mVertices[i].y;
		vertex[2] = mesh->mVertices[i].z;

		if (mesh->mNormals)
		{
			vertex[3] = mesh->mNormals[i].x;
			vertex[4] = mesh->mNormals[i].y;
			vertex[5] = mesh->mNormals[i].z;
		}
		else { vertex[3] = 0.f; vertex[4] = 0.f; vertex[5] = 1.f; };

		if (mesh->mTextureCoords[0])
		{
			vertex[6] = mesh->mTextureCoords[0][i].x;
			vertex[7] = mesh->mTextureCoords[0][i].y;
		}
		else { vertex[6] = 0.f; vertex[7] = 0.f; };

		destVertices->push_back(vertex);
	}

	delete[] vertex;

	// Get INDICES and store them.
	aiFace face;
	for (i = 0; i < mesh->mNumFaces; i++)
	{
		face = mesh->mFaces[i];
		for (j = 0; j < face.mNumIndices; j++)
			destIndices->push_back(face.mIndices[j]);
	}

	// Process material
	//if (mesh->mMaterialIndex >= 0)
	//{
	//	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	//	std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	//	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	//	std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	//	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	//}

	//return Mesh(vertices, indices, textures);
}


/*
std::vector<Texture> DataFromFile2::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<Texture> textures;

	for (unsigned i = 0; i < mat->GetTextureCount(type); i++) // check texture is already loaded
	{
		aiString str;
		mat->GetTexture(type, i, &str); // get texture file location
		bool skip = false;

		for (unsigned j = 0; j < textures_loaded.size(); j++)
		{
			textures.push_back(textures_loaded[j]);
			skip = true;
			break;
		}

		if (!skip)
		{
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), directory); // load texture using std_image.h
			texture.type = typeName;
			texture.path = str.C_Str();
			texture.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}
	return textures;
}
*/