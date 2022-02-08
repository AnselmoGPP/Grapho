
#include <string>
#include <unordered_map>		// For storing unique vertices from the model
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "loaddata.hpp"


VertexFromFile::VertexFromFile(const char* OBJfile)
{
	copyCString(this->OBJfile, OBJfile);
}

VertexFromFile::~VertexFromFile()
{

}

// (18)
void VertexFromFile::loadVertex(VertexSet& vertices, std::vector<uint32_t>& indices)
{
	// Load model
	tinyobj::attrib_t					 attrib;			// Holds all of the positions, normals and texture coordinates.
	std::vector<tinyobj::shape_t>		 shapes;			// Holds all of the separate objects and their faces. Each face consists of an array of vertices. Each vertex contains the indices of the position, normal and texture coordinate attributes.
	std::vector<tinyobj::material_t>	 materials;			// OBJ models can also define a material and texture per face, but we will ignore those.
	std::string							 warn, err;			// Errors and warnings that occur while loading the file.

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, OBJfile))
		throw std::runtime_error(warn + err);

	// Combine all the faces in the file into a single model
	std::unordered_map<VertexPCT, uint32_t> uniqueVertices{};	// Keeps track of the unique vertices and the respective indices, avoiding duplicated vertices (not indices).

	for (const auto& shape : shapes)
		for (const auto& index : shape.mesh.indices)
		{
			// Get each vertex
			VertexPCT vertex{};

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
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());	// Set new index for this vertex
				vertices.push_back(&vertex);										// Save vertex
			}

			// Save the index
			indices.push_back(uniqueVertices[vertex]);								// Save index
		}
}