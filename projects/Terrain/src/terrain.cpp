#include <algorithm>
#include <random>

#include "physics.hpp"

#include "terrain.hpp"
#include "ubo.hpp"


// Chunk ----------------------------------------------------------------------

Chunk::Chunk(Renderer& renderer, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned depth, unsigned chunkID)
    : renderer(renderer), 
    baseCenter(center),
    geoideCenter(center),
    groundCenter(center),   // defined more precisely in subclass' constructor
    stride(stride), 
    numHorVertex(numHorVertex), 
    numVertVertex(numVertVertex), 
    numAttribs(9), 
    vertexData(nullptr), 
    depth(depth), 
    modelOrdered(false), 
    isVisible(true), 
    chunkID(chunkID) { }

Chunk::~Chunk()
{
    //Problem: If Renderer is working (not at app closing), model must be erased here. (see also grass)
    //renderer.deleteModel(model);

    // Renderer already deletes all models when render loop ends (so this tries to delete already deleted models when application closes)
    // Problem: When application is closed, Renderer is destroyed. But destroying a chunk's model requires access to Renderer, so a crash may happen.
    //if (renderer.getModelsCount() && modelOrdered)
    //    renderer.deleteModel(model);
}

glm::vec3 Chunk::getVertex(size_t position) const
{ 
    return glm::vec3(
        vertex[position * numAttribs + 0], 
        vertex[position * numAttribs + 1], 
        vertex[position * numAttribs + 2] ); 
};

glm::vec3 Chunk::getNormal(size_t position) const
{ 
    return glm::vec3(
        vertex[position * numAttribs + 3], 
        vertex[position * numAttribs + 4], 
        vertex[position * numAttribs + 5] ); 
};

void Chunk::render(std::vector<ShaderLoader>& shaders, std::vector<TextureLoader>& textures, std::vector<uint16_t>* indices, unsigned numLights, bool transparency)
{
    // <<< Compute terrain and render here. No need to store vertices, indices or VertexInfo in Chunk object.

    vertexData = new VerticesLoader(
        vt_333.vertexSize,
        vertex.data(), 
        numHorVertex * numVertVertex, 
        indices ? *indices : this->indices);

    model = renderer.newModel(
        "chunk",
        1, 1, primitiveTopology::triangle, vt_333,
        *vertexData, shaders, textures,
        1, 4 * size.mat4 + 3 * size.vec4 + numLights * sizeof(LightPosDir),   // MM (mat4), VM (mat4), PM (mat4), MMN (mat3), camPos (vec3), time (float), n * LightPosDir (2*vec4), sideDepth (vec3)
        numLights * sizeof(LightProps),                                       // n * LightProps (6*vec4)
        transparency);

    uint8_t* dest;
    for (size_t i = 0; i < model->vsDynUBO.numDynUBOs; i++)
    {
        dest = model->vsDynUBO.getUBOptr(i);
        memcpy(dest, &getModelMatrix(), size.mat4);
        dest += size.mat4;
        //memcpy(dest, &view, mat4size);
        dest += size.mat4;
        //memcpy(dest, &proj, mat4size);
        dest += size.mat4;
        memcpy(dest, &getModelMatrixForNormals(getModelMatrix()), size.mat4);
        dest += size.mat4;
        //memcpy(dest, &camPos, vec3size);
        //dest += vec4size;
        //memcpy(dest, &time, sizeof(float));
        //dest += vec4size;
        //memcpy(dest, &sideDepths, vec4size);
        //dest += vec4size;
        //memcpy(dest, lights.posDir, lights.posDirBytes);
        //dest += lights.posDirBytes;
    }

    //dest = model->fsUBO.getUBOptr(0);
    //memcpy(dest, lights.props, lights.propsBytes);
    //dest += lights.propsBytes;

    modelOrdered = true;

    
}

void Chunk::updateUBOs(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos, const LightSet& lights, float time, float camHeight, glm::vec3 planetCenter)
{
    if (!modelOrdered) return;

    uint8_t* dest;

    for (size_t i = 0; i < model->vsDynUBO.numDynUBOs; i++)
    {
        dest = model->vsDynUBO.getUBOptr(i);

        //memcpy(dest, &modelMatrix(), size.mat4);
        dest += size.mat4;
        memcpy(dest, &view, size.mat4);
        dest += size.mat4;
        memcpy(dest, &proj, size.mat4);
        dest += size.mat4;
        //memcpy(dest, &modelMatrixForNormals(modelMatrix()), size.mat4);
        dest += size.mat4;
        memcpy(dest, &camPos, size.vec3);
        dest += size.vec4;
        memcpy(dest, &time, sizeof(float));
        dest += sizeof(float);
        memcpy(dest, &camHeight, sizeof(float));
        dest += 3 * sizeof(float);
        memcpy(dest, &sideDepths, size.vec4);
        dest += size.vec4;
        memcpy(dest, lights.posDir, lights.posDirBytes);
        //dest += lights.posDirBytes;
    }

    dest = model->fsUBO.getUBOptr(0);
    memcpy(dest, lights.props, lights.propsBytes);
    //dest += lights.propsBytes;
}

void Chunk::computeIndices(std::vector<uint16_t>& indices, unsigned numHorVertex, unsigned numVertVertex)
{
    indices.resize((numHorVertex - 1) * (numVertVertex - 1) * 2 * 3);

    for (size_t v = 0; v < numVertVertex - 1; v++)
        for (size_t h = 0; h < numHorVertex - 1; h++)
        {
            unsigned int pos = v * numHorVertex + h;

            indices.push_back(pos);
            indices.push_back(pos + numHorVertex + 1);
            indices.push_back(pos + numHorVertex);

            indices.push_back(pos);
            indices.push_back(pos + 1);
            indices.push_back(pos + numHorVertex + 1);
        }
}

void Chunk::setSideDepths(unsigned a, unsigned b, unsigned c, unsigned d)
{
    sideDepths[0] = a;
    sideDepths[1] = b;
    sideDepths[2] = c;
    sideDepths[3] = d;
}

glm::vec3 Chunk::getCenter() { return groundCenter; }

void Chunk::deleteModel() { renderer.deleteModel(model); }


// PlainChunk ----------------------------------------------------------------------

PlainChunk::PlainChunk(Renderer& renderer, Noiser* noiseGenerator, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned depth, unsigned chunkID)
    : Chunk(renderer, center, stride, numHorVertex, numVertVertex, depth, chunkID), noiseGen(noiseGenerator)
{
    groundCenter.z = noiseGen->getNoise(baseCenter.x, baseCenter.y);

    computeSizes();
}

void PlainChunk::computeTerrain(bool computeIndices)   // <<< Fix function to make it similar to SphericalChunk::computeTerrain()
{
    size_t index;
    float x0 = baseCenter.x - horChunkSize / 2;
    float y0 = baseCenter.y - vertChunkSize / 2;

    // Vertex data
    vertex.resize(numHorVertex * numVertVertex * 6);

    for (size_t y = 0; y < numVertVertex; y++)
        for (size_t x = 0; x < numHorVertex; x++)
        {
            index = y * numHorVertex + x;

            // Positions (0, 1, 2)
            vertex[index * 6 + 0] = x0 + x * stride;
            vertex[index * 6 + 1] = y0 + y * stride;
            vertex[index * 6 + 2] = noiseGen->getNoise((float)vertex[index * 6 + 0], (float)vertex[index * 6 + 1]);
        }

    // Normals (3, 4, 5)
    computeGridNormals();

    // Indices
    if (computeIndices)
        this->computeIndices(indices, numHorVertex, numVertVertex);
}

void PlainChunk::computeGridNormals()   // <<< Fix function to make it similar to SphericalChunk::computeGridNormals()
{
    // Initialize normals to 0
    unsigned numVertex = numHorVertex * numVertVertex;
    std::vector<glm::vec3> tempNormals(numVertex, glm::vec3(0.f, 0.f, 0.f));
    for (size_t i = 0; i < numVertex; i++) tempNormals[i] = glm::vec3(0.f, 0.f, 0.f);

    // Compute normals
    for (size_t y = 0; y < numVertVertex - 1; y++)
        for (size_t x = 0; x < numHorVertex - 1; x++)
        {
            /*
                In each iteration, we operate in each square of the grid (4 vertex):

                           Cside
                       (D)------>(C)
                        |         |
                  Dside |         | Bside
                        v         v
                       (A)------>(B)
                           Aside
             */

             // Vertex positions in the array
            size_t posA = y * numHorVertex + x;
            size_t posB = y * numHorVertex + (x + 1);
            size_t posC = (y + 1) * numHorVertex + (x + 1);
            size_t posD = (y + 1) * numHorVertex + x;

            // Vertex vectors
            glm::vec3 A = getVertex(posA);
            glm::vec3 B = getVertex(posB);
            glm::vec3 C = getVertex(posC);
            glm::vec3 D = getVertex(posD);

            // Vector representing each side
            glm::vec3 Aside = B - A;
            glm::vec3 Bside = B - C;
            glm::vec3 Cside = C - D;
            glm::vec3 Dside = A - D;

            // Normal computed for each vertex from the two side vectors it has attached
            glm::vec3 Anormal = glm::cross(Aside, -Dside);
            glm::vec3 Bnormal = glm::cross(-Bside, -Aside);
            glm::vec3 Cnormal = glm::cross(-Cside, Bside);
            glm::vec3 Dnormal = glm::cross(Dside, Cside);

            // Add to the existing normal of the vertex
            tempNormals[posA] += Anormal;
            tempNormals[posB] += Bnormal;
            tempNormals[posC] += Cnormal;
            tempNormals[posD] += Dnormal;
        }

    // Normalize the normals (normalized in shader)
    for (int i = 0; i < numVertex; i++)
    {
        tempNormals[i] = glm::normalize(tempNormals[i]);

        vertex[i * 6 + 3] = tempNormals[i].x;
        vertex[i * 6 + 4] = tempNormals[i].y;
        vertex[i * 6 + 5] = tempNormals[i].z;
    }
}

void PlainChunk::getSubBaseCenters(std::tuple<float, float, float>* centers)
{
    float quarterSide = horChunkSize / 4;

    centers[0] = std::tuple(baseCenter.x - quarterSide, baseCenter.y + quarterSide, baseCenter.z);
    centers[1] = std::tuple(baseCenter.x + quarterSide, baseCenter.y + quarterSide, baseCenter.z);
    centers[2] = std::tuple(baseCenter.x - quarterSide, baseCenter.y - quarterSide, baseCenter.z);
    centers[3] = std::tuple(baseCenter.x + quarterSide, baseCenter.y - quarterSide, baseCenter.z);
}

void PlainChunk::computeSizes()
{
    horBaseSize = stride * (numHorVertex - 1);
    vertBaseSize = stride * (numVertVertex - 1);

    horChunkSize = horBaseSize;
    vertChunkSize = vertBaseSize;
}

// PlanetChunk ----------------------------------------------------------------------

PlanetChunk::PlanetChunk(Renderer& renderer, std::shared_ptr<Noiser> noiseGenerator, glm::vec3 cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned depth, unsigned chunkID)
    : Chunk(renderer, cubeSideCenter, stride, numHorVertex, numVertVertex, depth, chunkID), noiseGen(noiseGenerator), nucleus(nucleus), radius(radius)
{
    glm::vec3 unitVec = glm::normalize(baseCenter - nucleus);
    geoideCenter = unitVec * radius;
    if(noiseGenerator) groundCenter = geoideCenter + unitVec * noiseGen->getNoise(geoideCenter.x, geoideCenter.y, geoideCenter.z);  // if added due to SphereChunk

    // Set relative axes of the cube face (needed for computing indices in good order)
     if (cubePlane.x != 0)           // 1: (y, z)  // -1: (-y, z)
    {
        xAxis = glm::vec3(0, cubePlane.x, 0);
        yAxis = glm::vec3(0, 0, 1);
    }
    else if (cubePlane.y != 0)      // 1: (-x, z)  // -1: (x, z)
    {
        xAxis = glm::vec3(-cubePlane.y, 0, 0);
        yAxis = glm::vec3(0, 0, 1);
    }
    else if (cubePlane.z != 0)       // 1: (x, y)  // -1: (-x, y)
    {
        xAxis = glm::vec3(cubePlane.z, 0, 0);
        yAxis = glm::vec3(0, 1, 0);
    }
    //else std::cout << "cubePlane parameter has wrong format" << std::endl;   // cubePlane must contain 2 zeros

    computeSizes();
}

void PlanetChunk::computeTerrain(bool computeIndices)
{
    // Vertex data (+ frame)
    glm::vec3 pos0 = baseCenter - (xAxis * horBaseSize / 2.f + yAxis * vertBaseSize / 2.f);   // Position of the initial coordinate in the cube side plane (lower left).
    pos0 -= (xAxis * stride + yAxis * stride);      // Set frame
    unsigned tempNumHorV = numHorVertex  + 2;
    unsigned tempNumVerV = numVertVertex + 2;
    vertex.resize(tempNumHorV * tempNumVerV * numAttribs);
    glm::vec3 unitVec, cube, sphere, ground;
    size_t index;

    for (size_t v = 0; v < tempNumVerV; v++)
        for (size_t h = 0; h < tempNumHorV; h++)
        {
            index = (v * tempNumHorV + h) * numAttribs;

            // Positions (0, 1, 2)
            cube = pos0 + (xAxis * (float)h * stride) + (yAxis * (float)v * stride);
            unitVec = glm::normalize(cube - nucleus);
            sphere = unitVec * radius;
            ground = sphere + unitVec * noiseGen->getNoise(sphere.x, sphere.y, sphere.z);
            vertex[index + 0] = ground.x;
            vertex[index + 1] = ground.y;
            vertex[index + 2] = ground.z;
            vertex[index + 6] = 0;          // Vertex type (default = 0)
        }
    
    // Normals (3, 4, 5) (+ frame)
    computeGridNormals(pos0, xAxis, yAxis, tempNumHorV, tempNumVerV);

    // Crop frame (relocate vertices in the vector and crop it)
    size_t i = 0, j = 0;
    for (size_t v = 1; v < (tempNumVerV - 1); v++)
        for (size_t h = 1; h < (tempNumHorV - 1); h++)
        {
            index = (v * tempNumHorV + h) * numAttribs;

            for(j = 0; j < numAttribs; j++)
                vertex[i++] = vertex[index + j];
        }
    vertex.resize(numHorVertex * numVertVertex * numAttribs);

    // Compute gap-fixing data (6, 7, 8).
    computeGapFixes();

    // Indices
    if (computeIndices)
        this->computeIndices(indices, numHorVertex, numVertVertex);
}

void PlanetChunk::computeGridNormals(glm::vec3 pos0, glm::vec3 xAxis, glm::vec3 yAxis, unsigned numHorV, unsigned numVerV)
{
    // Initialize normals to 0
    unsigned numVertex = numHorV * numVerV;
    std::vector<glm::vec3> tempNormals(numVertex, glm::vec3(0));

    size_t posA, posB, posC, posD;
    glm::vec3 A, B, C, D;
    glm::vec3 Aside, Bside, Cside, Dside;
    glm::vec3 Anormal, Bnormal, Cnormal, Dnormal;

    // Compute normals
    for (size_t y = 0; y < (numVerV - 1); y++)
        for (size_t x = 0; x < (numHorV - 1); x++)
        {
            /*
                In each iteration, we operate in each square of the grid (4 vertex):

                           Cside
                       (D)------>(C)
                        |         |
                  Dside |         | Bside
                        v         v
                       (A)------>(B)
                           Aside
             */

            // Vertex positions in the array
            posA = y * numHorV + x;
            posB = y * numHorV + (x+1);
            posC = (y+1) * numHorV + (x+1);
            posD = (y+1) * numHorV + x;

            // Vertex world coordinates
            A = getVertex(posA);
            B = getVertex(posB);
            C = getVertex(posC);
            D = getVertex(posD);

            // Vector representing each side
            Aside = B - A;
            Bside = B - C;
            Cside = C - D;
            Dside = A - D;

            // Normal computed for each vertex from the two side vectors it has attached
            Anormal = glm::cross(Aside, -Dside);
            Bnormal = glm::cross(-Bside, -Aside);
            Cnormal = glm::cross(-Cside, Bside);
            Dnormal = glm::cross(Dside, Cside);

            // Add to the existing normal of the vertex
            tempNormals[posA] += Anormal;
            tempNormals[posB] += Bnormal;
            tempNormals[posC] += Cnormal;
            tempNormals[posD] += Dnormal;
        }

    // Normalize the normals
    size_t j;
    for (size_t i = 0; i < numVertex; i++)
    {
        j = i * numAttribs;

        tempNormals[i] = glm::normalize(tempNormals[i]);
        vertex[j + 3] = tempNormals[i].x;
        vertex[j + 4] = tempNormals[i].y;
        vertex[j + 5] = tempNormals[i].z;
    }
}

void PlanetChunk::getSubBaseCenters(std::tuple<float, float, float>* centers)
{
    float quarterSide = horBaseSize / 4;
    glm::vec3 vecs[4];

    vecs[0] = baseCenter - (quarterSide * xAxis) + (quarterSide * yAxis);
    vecs[1] = baseCenter + (quarterSide * xAxis) + (quarterSide * yAxis);
    vecs[2] = baseCenter - (quarterSide * xAxis) - (quarterSide * yAxis);
    vecs[3] = baseCenter + (quarterSide * xAxis) - (quarterSide * yAxis);

    centers[0] = std::tuple(vecs[0].x, vecs[0].y, vecs[0].z);
    centers[1] = std::tuple(vecs[1].x, vecs[1].y, vecs[1].z);
    centers[2] = std::tuple(vecs[2].x, vecs[2].y, vecs[2].z);
    centers[3] = std::tuple(vecs[3].x, vecs[3].y, vecs[3].z);
}

float PlanetChunk::getRadius() { return radius; }

void PlanetChunk::computeSizes()
{
    horBaseSize = stride * (numHorVertex - 1);
    vertBaseSize = stride * (numVertVertex - 1);

    glm::vec3 left = radius * glm::normalize(baseCenter - (xAxis * horBaseSize / 2.f) - nucleus);
    glm::vec3 right = radius * glm::normalize(baseCenter + (xAxis * horBaseSize / 2.f) - nucleus);
    glm::vec3 bottom = radius * glm::normalize(baseCenter - (yAxis * vertBaseSize / 2.f) - nucleus);
    glm::vec3 top = radius * glm::normalize(baseCenter + (yAxis * vertBaseSize / 2.f) - nucleus);
    horChunkSize = glm::length(left - right);
    vertChunkSize = glm::length(bottom - top);
}

void PlanetChunk::computeGapFixes()
{
    // Attributes: 6 (vertex type: 0,1,2,3,4), 7 (extra height for x2 difference), 8 (for x4 difference).
    // Shader: If vertex type != 0 or 1000, fix vertex position (if required) using side depths (uniform) and extra height (attribute).

    glm::vec3 current, average;
    unsigned remain, prev, next;
    float ratio;
    size_t index;

    for (size_t v = 1; v < numVertVertex - 1; v++)
        if (v % 4)
        {
            if (v % 2)
            {
                // x2 left
                index = (numHorVertex * v) * numAttribs;
                prev = index - numHorVertex * numAttribs;
                next = index + numHorVertex * numAttribs;
                current = glm::vec3(vertex[index + 0], vertex[index + 1], vertex[index + 2]);
                average.x = (vertex[prev + 0] + vertex[next + 0]) / 2;
                average.y = (vertex[prev + 1] + vertex[next + 1]) / 2;
                average.z = (vertex[prev + 2] + vertex[next + 2]) / 2;
                vertex[index + 6] = side::left + 1;
                vertex[index + 7] = glm::length(average - nucleus) - glm::length(current - nucleus);

                // x2 right
                index = (numHorVertex * (v + 1) - 1) * numAttribs;
                prev = index - numHorVertex * numAttribs;
                next = index + numHorVertex * numAttribs;
                current = glm::vec3(vertex[index + 0], vertex[index + 1], vertex[index + 2]);
                average.x = (vertex[prev + 0] + vertex[next + 0]) / 2;
                average.y = (vertex[prev + 1] + vertex[next + 1]) / 2;
                average.z = (vertex[prev + 2] + vertex[next + 2]) / 2;
                vertex[index + 6] = side::right + 1;
                vertex[index + 7] = glm::length(average - nucleus) - glm::length(current - nucleus);
            }

            remain = v % 4;
            ratio = remain / 4.f;

            // x4 left
            index = (numHorVertex * v) * numAttribs;
            prev = index - numHorVertex * numAttribs * remain;
            next = index + numHorVertex * numAttribs * (4 - remain);
            current = glm::vec3(vertex[index + 0], vertex[index + 1], vertex[index + 2]);
            average.x = vertex[prev + 0] + (vertex[next + 0] - vertex[prev + 0]) * ratio;
            average.y = vertex[prev + 1] + (vertex[next + 1] - vertex[prev + 1]) * ratio;
            average.z = vertex[prev + 2] + (vertex[next + 2] - vertex[prev + 2]) * ratio;
            vertex[index + 6] = side::left + 1;
            vertex[index + 8] = glm::length(average - nucleus) - glm::length(current - nucleus);

            // x4 right
            index = (numHorVertex * (v + 1) - 1) * numAttribs;
            prev = index - numHorVertex * numAttribs * remain;
            next = index + numHorVertex * numAttribs * (4 - remain);
            current = glm::vec3(vertex[index + 0], vertex[index + 1], vertex[index + 2]);
            average.x = vertex[prev + 0] + (vertex[next + 0] - vertex[prev + 0]) * ratio;
            average.y = vertex[prev + 1] + (vertex[next + 1] - vertex[prev + 1]) * ratio;
            average.z = vertex[prev + 2] + (vertex[next + 2] - vertex[prev + 2]) * ratio;
            vertex[index + 6] = side::right + 1;
            vertex[index + 8] = glm::length(average - nucleus) - glm::length(current - nucleus);
        }

    for (size_t h = 1; h < numHorVertex - 1; h++)
        if (h % 4)
        {
            if (h % 2)
            {
                // x2 up
                index = (numHorVertex * (numVertVertex - 1) + h) * numAttribs;
                prev = index - numAttribs;
                next = index + numAttribs;
                current = glm::vec3(vertex[index + 0], vertex[index + 1], vertex[index + 2]);
                average.x = (vertex[prev + 0] + vertex[next + 0]) / 2;
                average.y = (vertex[prev + 1] + vertex[next + 1]) / 2;
                average.z = (vertex[prev + 2] + vertex[next + 2]) / 2;
                vertex[index + 6] = side::up + 1;
                vertex[index + 7] = glm::length(average - nucleus) - glm::length(current - nucleus);
                
                // x2 down
                index = h * numAttribs;
                prev = index - numAttribs;
                next = index + numAttribs;
                current = glm::vec3(vertex[index + 0], vertex[index + 1], vertex[index + 2]);
                average.x = (vertex[prev + 0] + vertex[next + 0]) / 2;
                average.y = (vertex[prev + 1] + vertex[next + 1]) / 2;
                average.z = (vertex[prev + 2] + vertex[next + 2]) / 2;
                vertex[index + 6] = side::down + 1;
                vertex[index + 7] = glm::length(average - nucleus) - glm::length(current - nucleus);
            }

            remain = h % 4;
            ratio = remain / 4.f;

            // x4 up
            index = (numHorVertex * (numVertVertex - 1) + h) * numAttribs;
            prev = index - remain * numAttribs;
            next = index + (4 - remain) * numAttribs;
            current = glm::vec3(vertex[index + 0], vertex[index + 1], vertex[index + 2]);
            average.x = vertex[prev + 0] + (vertex[next + 0] - vertex[prev + 0]) * ratio;
            average.y = vertex[prev + 1] + (vertex[next + 1] - vertex[prev + 1]) * ratio;
            average.z = vertex[prev + 2] + (vertex[next + 2] - vertex[prev + 2]) * ratio;
            vertex[index + 6] = side::up + 1;
            vertex[index + 8] = glm::length(average - nucleus) - glm::length(current - nucleus);

            // x4 down
            index = h * numAttribs;
            prev = index - remain * numAttribs;
            next = index + (4 - remain) * numAttribs;
            current = glm::vec3(vertex[index + 0], vertex[index + 1], vertex[index + 2]);
            average.x = vertex[prev + 0] + (vertex[next + 0] - vertex[prev + 0]) * ratio;
            average.y = vertex[prev + 1] + (vertex[next + 1] - vertex[prev + 1]) * ratio;
            average.z = vertex[prev + 2] + (vertex[next + 2] - vertex[prev + 2]) * ratio;
            vertex[index + 6] = side::down + 1;
            vertex[index + 8] = glm::length(average - nucleus) - glm::length(current - nucleus);
        }
}

// SphereChunk ----------------------------------------------------------------------

SphereChunk::SphereChunk(Renderer& renderer, glm::vec3 cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned depth, unsigned chunkID)
    : PlanetChunk(renderer, nullptr, cubeSideCenter, stride, numHorVertex, numVertVertex, radius, nucleus, cubePlane, depth, chunkID)
{
    geoideCenter = glm::normalize(baseCenter - nucleus) * radius;
    groundCenter = geoideCenter;
}

void SphereChunk::computeTerrain(bool computeIndices)
{
    // Vertex data (+ frame)
    glm::vec3 pos0 = baseCenter - (xAxis * horBaseSize / 2.f + yAxis * vertBaseSize / 2.f);   // Position of the initial coordinate in the cube side plane (lower left).
    pos0 -= (xAxis * stride + yAxis * stride);      // Set frame
    unsigned tempNumHorV = numHorVertex + 2;
    unsigned tempNumVerV = numVertVertex + 2;
    vertex.resize(tempNumHorV * tempNumVerV * numAttribs);
    glm::vec3 unitVec, cube, ground;
    size_t index;

    for (size_t v = 0; v < tempNumVerV; v++)
        for (size_t h = 0; h < tempNumHorV; h++)
        {
            index = (v * tempNumHorV + h) * numAttribs;

            // Positions (0, 1, 2)
            cube = pos0 + (xAxis * (float)h * stride) + (yAxis * (float)v * stride);
            ground = glm::normalize(cube - nucleus) * radius;
            vertex[index + 0] = ground.x;
            vertex[index + 1] = ground.y;
            vertex[index + 2] = ground.z;
            vertex[index + 6] = 0;          // Vertex type (default = 0)
        }

    // Normals (3, 4, 5) (+ frame)
    computeGridNormals(pos0, xAxis, yAxis, tempNumHorV, tempNumVerV);

    // Crop frame (relocate vertices in the vector and crop it)
    size_t i = 0, j = 0;
    for (size_t v = 1; v < (tempNumVerV - 1); v++)
        for (size_t h = 1; h < (tempNumHorV - 1); h++)
        {
            index = (v * tempNumHorV + h) * numAttribs;

            for (j = 0; j < numAttribs; j++)
                vertex[i++] = vertex[index + j];
        }
    vertex.resize(numHorVertex * numVertVertex * numAttribs);

    // Compute gap-fixing data (6, 7, 8).
    computeGapFixes();

    // Indices
    if (computeIndices)
        this->computeIndices(indices, numHorVertex, numVertVertex);
}

// DynamicGrid ----------------------------------------------------------------------

DynamicGrid::DynamicGrid(glm::vec3 camPos, Renderer* renderer, unsigned activeTree, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, bool transparency)
    : camPos(camPos),
    numLights(0),
    renderer(renderer), 
    activeTree(activeTree),
    nonActiveTree((activeTree + 1) % 2),
    rootCellSize(rootCellSize), 
    numSideVertex(numSideVertex), 
    numLevels(numLevels), 
    minLevel(minLevel), 
    distMultiplier(distMultiplier),
    distMultRemove(distMultiplier * 2),
    transparency(transparency)
{
    root[0] = root[1] = nullptr;
    Chunk::computeIndices(indices, numSideVertex, numSideVertex);
};

DynamicGrid::~DynamicGrid()
{
    if (root[0]) delete root[0];
    if (root[1]) delete root[1];

    for (auto it = chunks.begin(); it != chunks.end(); it++)
        delete it->second;

    chunks.clear();
}

void DynamicGrid::addResources(const std::vector<ShaderLoader>& shadersInfo, const std::vector<TextureLoader>& texturesInfo)
{
    this->shaders = shadersInfo;
    this->textures = texturesInfo;
}

void DynamicGrid::updateTree(glm::vec3 newCamPos, unsigned numLights)
{
    if (!numLevels) return;

    // Return if CamPos has not changed, Active tree exists, and Non-active tree is nullptr.
    if (root[activeTree] && !root[nonActiveTree] && camPos.x == newCamPos.x && camPos.y == newCamPos.y && camPos.z == newCamPos.z)
        return;  // ERROR: When updateTree doesn't run in each frame (i.e., when command buffer isn't created each frame), no validation error appears after resizing window
    
    // Build tree if the non-active tree is nullptr
    if (!root[nonActiveTree])
    {
        camPos = newCamPos;
        this->numLights = numLights;
        updateVisibilityState();        // overridden method

        visibleLeafChunks[nonActiveTree].clear();
        root[nonActiveTree] = getNode(closestCenter(), rootCellSize, 0, 1);
        createTree(root[nonActiveTree], 0);                         // Build tree and load leaf-chunks
    }

    // <<< Why is executed each time I move? Why is only executed in one plane for fixing gaps?
    // Check whether non-active tree has fully constructed leaf-chunks. If so, switch trees
    if (fullConstChunks(nonActiveTree))
    {
        updateChunksSideDepths(root[nonActiveTree]);
        
        changeRenders(activeTree, false);
        if (root[activeTree]) delete root[activeTree];
        root[activeTree] = nullptr;
        
        changeRenders(nonActiveTree, true);

        removeFarChunks(distMultRemove, newCamPos);

        resetVisibility(root[nonActiveTree]);

        std::swap(activeTree, nonActiveTree);
    }
}

void DynamicGrid::createTree(QuadNode<Chunk*>* node, size_t depth)
{
    Chunk* chunk = node->getElement();
    glm::vec3 gCenter = getChunkCenter(chunk);
    float dist = sqrt((camPos.x - gCenter.x) * (camPos.x - gCenter.x) + (camPos.y - gCenter.y) * (camPos.y - gCenter.y) + (camPos.z - gCenter.z) * (camPos.z - gCenter.z));

    // Is leaf node > Compute terrain > Children are nullptr by default
    if (depth >= minLevel && (dist > chunk->getHorChunkSide() * distMultiplier || depth == numLevels - 1))// <<<<<<< squaring distances is correct here?
    {
        if (!isVisible(chunk)) { 
            node->getElement()->isVisible = false; 
            return; 
        }

        if (chunk->modelOrdered == false) {
            chunk->computeTerrain(false);
            chunk->render(shaders, textures, &indices, numLights, transparency);
            renderer->setRenders(chunk->model, 0);
        }

        visibleLeafChunks[nonActiveTree].push_back(node->getElement());
    }
    // Is not leaf node > Create children > Recursion
    else
    {
        depth++;
        std::tuple<float, float, float> subBaseCenters[4];
        chunk->getSubBaseCenters(subBaseCenters);
        float halfSide = chunk->getHorBaseSide() / 2;
        glm::vec4 chunkIDs = getChunkIDs(chunk->chunkID, depth);
        
        node->setA(getNode(subBaseCenters[0], halfSide, depth, chunkIDs[0]));    // - x + y
        createTree(node->getA(), depth);

        node->setB(getNode(subBaseCenters[1], halfSide, depth, chunkIDs[1]));    // + x + y
        createTree(node->getB(), depth);

        node->setC(getNode(subBaseCenters[2], halfSide, depth, chunkIDs[2]));    // - x - y
        createTree(node->getC(), depth);

        node->setD(getNode(subBaseCenters[3], halfSide, depth, chunkIDs[3]));    // + x - y
        createTree(node->getD(), depth);
    }
}

bool DynamicGrid::fullConstChunks(unsigned treeIndex)
{
    for (Chunk* chunk : visibleLeafChunks[treeIndex])
        if (!chunk->model->fullyConstructed)
            return false;

    return true;
}

void DynamicGrid::updateChunksSideDepths(QuadNode<Chunk*>* node)
{
    restartSideDepths(node);                                    // Set nodes' side depths to 0
    node->getElement()->setSideDepths(1000, 1000, 1000, 1000);  // Initialize root side depths to 1000 (flag for grid boundaries). The rest of nodes have side depths of 0 (set previously). 

    QuadNode<Chunk*>* currentNode;
    std::list<Chunk*> allLeaves;
    std::list<QuadNode<Chunk*>*> queue;
    queue.push_back(node);

    // Get depths of neighbour chunks (breadth-first traversal) (result = depth each side should keep to make its borders fit)
    while (queue.size())
    {
        // Dequeue firt element
        currentNode = queue.front();
        queue.pop_front();

        // Modify data
        updateChunksSideDepths_help(queue, currentNode);
        if (currentNode->isLeaf())
            allLeaves.push_back(currentNode->getElement());

        // Enqueue childs
        if (!currentNode->isLeaf())
        {
            queue.push_back(currentNode->getA());
            queue.push_back(currentNode->getB());
            queue.push_back(currentNode->getC());
            queue.push_back(currentNode->getD());
        }
    }

    // Get depth differences of neigbour leaves and pass them as UBO
    for (auto& it : allLeaves)
    {
        if (it->sideDepths[0] != 1000) it->sideDepths[0] = it->depth - it->sideDepths[0];
        if (it->sideDepths[1] != 1000) it->sideDepths[1] = it->depth - it->sideDepths[1];
        if (it->sideDepths[2] != 1000) it->sideDepths[2] = it->depth - it->sideDepths[2];
        if (it->sideDepths[3] != 1000) it->sideDepths[3] = it->depth - it->sideDepths[3];
    }
}

void DynamicGrid::changeRenders(unsigned treeIndex, bool renderMode)
{
    for (Chunk*& chunk : visibleLeafChunks[treeIndex])
        renderer->setRenders(chunk->model, renderMode);
}

void DynamicGrid::resetVisibility(QuadNode<Chunk*>* node)
{
    if (!node) return;

    if (node->isLeaf())
        node->getElement()->isVisible = true;
    else
    {
        resetVisibility(node->getA());
        resetVisibility(node->getB());
        resetVisibility(node->getC());
        resetVisibility(node->getD());
    }
}

void DynamicGrid::removeFarChunks(unsigned relDist, glm::vec3 camPosNow)
{
    glm::vec3 center;
    glm::vec3 distVec;
    float targetDist;
    float targetSqrDist;
    std::map<std::tuple<float, float, float>, Chunk*>::iterator it = chunks.begin();
    std::map<std::tuple<float, float, float>, Chunk*>::iterator nextIt;
    Chunk* chunk;

    while (it != chunks.end())
    {
        nextIt = it;
        nextIt++;
        targetDist = it->second->getHorChunkSide() * relDist;
        chunk = it->second;
        
        if(chunk->isVisible && (!chunk->modelOrdered || !chunk->model->activeRenders))    // traverse chunks that are not in the active tree
        {
            center = getChunkCenter(it->second);    // it->second->getGroundCenter();
            distVec.x = center.x - camPosNow.x;
            distVec.y = center.y - camPosNow.y;
            distVec.z = center.z - camPosNow.z;
            targetSqrDist = targetDist * targetDist;

            if ((distVec.x * distVec.x + distVec.y * distVec.y + distVec.z * distVec.z) > targetSqrDist)
            {
                if(chunks[it->first]->modelOrdered) chunks[it->first]->deleteModel();
                delete chunks[it->first];
                chunks.erase(it->first);
            }
        }

        it = nextIt;
    }
}

void DynamicGrid::updateUBOs(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos, const LightSet& lights, float time, float groundHeight)
{
    this->camPos = camPos;

    for (Chunk* chunk : visibleLeafChunks[activeTree])
        chunk->updateUBOs(view, proj, camPos, lights, time, groundHeight);
}

void DynamicGrid::toLastDraw() { putToLastDraw(activeTree); }

void DynamicGrid::putToLastDraw(unsigned treeIndex)
{
    for (Chunk* chunk : visibleLeafChunks[treeIndex])
        renderer->toLastDraw(chunk->model);
}

void DynamicGrid::updateChunksSideDepths_help(std::list<QuadNode<Chunk*>*>& queue, QuadNode<Chunk*>* currentNode)
{   
    /*
        Setting side depths of a chunk requires knowing side depths of neighbor chunks, so we use breath-first search.
        Each step updates some chunks' side depths: current chunk (right & down), right chunk (left), lower chunk (up).
        When a depth level starts to be checked, the whole depth level is already in the queue.
        Due to the way the tree was built, the traversal goes left-right up-down through the grid.
        Convention: At the beginning, all side depths are 0, but root chunk ones are 1000 (flag for grid boundaries).
        Process:
            - Get right and lower node
            - If any of the 3 nodes is leaf, update corresponding side depths (if they are 0).
            - Compare adjacent side depths between the 3 of them and update them if required.
            - Pass side depths from current chunk to its children (if it's not leaf).
    */

    Chunk * currentChunk = currentNode->getElement(), * rightChunk = nullptr, * lowerChunk = nullptr;
    QuadNode<Chunk*>* rightNode = nullptr, * lowerNode = nullptr;
    unsigned sideSize = pow(2, currentChunk->depth);

    // Get right node
    for (auto iter = queue.begin(); iter != queue.end(); iter++)
    {
        if ((*iter)->getElement()->chunkID == currentChunk->chunkID + 1 && (*iter)->getElement()->depth == currentChunk->depth)
        {
            rightNode = *iter;
            rightChunk = rightNode->getElement();
            break;
        }
        else if ((*iter)->getElement()->depth != currentChunk->depth)
            break;
    }

    // Get lower node
    for (auto iter = queue.begin(); iter != queue.end(); iter++)
        if ((*iter)->getElement()->chunkID == currentChunk->chunkID + sideSize && (*iter)->getElement()->depth == currentChunk->depth)
        {
            lowerNode = *iter;
            lowerChunk = lowerNode->getElement();
            break;
        }
        else if ((*iter)->getElement()->depth != currentChunk->depth)
            break;
    
    // Modify side depths
    if (currentNode->isLeaf())
    {
        if (!currentChunk->sideDepths[side::right]) currentChunk->sideDepths[side::right] = currentChunk->depth;
        if (!currentChunk->sideDepths[side::down]) currentChunk->sideDepths[side::down] = currentChunk->depth;
    }

    if (rightNode)
    {
        if (rightNode->isLeaf() && !rightChunk->sideDepths[side::left])
            rightChunk->sideDepths[side::left] = currentChunk->depth;

        if (currentChunk->sideDepths[side::right] && !rightChunk->sideDepths[side::left])
            rightChunk->sideDepths[side::left] = currentChunk->sideDepths[side::right];
        if (!currentChunk->sideDepths[side::right] && rightChunk->sideDepths[side::left])
            currentChunk->sideDepths[side::right] = rightChunk->sideDepths[side::left];
    }

    if (lowerNode)
    {
        if (lowerNode->isLeaf() && !lowerChunk->sideDepths[side::up])
            lowerChunk->sideDepths[side::up] = currentChunk->depth;

        if (currentChunk->sideDepths[side::down] && !lowerChunk->sideDepths[side::up])
            lowerChunk->sideDepths[side::up] = currentChunk->sideDepths[side::down];
        if (!currentChunk->sideDepths[side::down] && lowerChunk->sideDepths[side::up])
            currentChunk->sideDepths[side::down] = lowerChunk->sideDepths[side::up];
    }
    
    // Pass sides from parent to children
    if (!currentNode->isLeaf())
    {
        currentNode->getA()->getElement()->sideDepths[side::left ] = currentChunk->sideDepths[side::left ];
        currentNode->getC()->getElement()->sideDepths[side::left ] = currentChunk->sideDepths[side::left ];
        currentNode->getB()->getElement()->sideDepths[side::right] = currentChunk->sideDepths[side::right];
        currentNode->getD()->getElement()->sideDepths[side::right] = currentChunk->sideDepths[side::right];
        currentNode->getA()->getElement()->sideDepths[side::up   ] = currentChunk->sideDepths[side::up   ];
        currentNode->getB()->getElement()->sideDepths[side::up   ] = currentChunk->sideDepths[side::up   ];
        currentNode->getC()->getElement()->sideDepths[side::down ] = currentChunk->sideDepths[side::down ];
        currentNode->getD()->getElement()->sideDepths[side::down ] = currentChunk->sideDepths[side::down ];
    }
}

void DynamicGrid::restartSideDepths(QuadNode<Chunk*>* node)
{
    if (!node) return;

    node->getElement()->setSideDepths(0, 0, 0, 0);

    restartSideDepths(node->getA());
    restartSideDepths(node->getB());
    restartSideDepths(node->getC());
    restartSideDepths(node->getD());
}

glm::vec3 DynamicGrid::getChunkCenter(Chunk* chunk) { return chunk->getGroundCenter(); }

glm::vec4 DynamicGrid::getChunkIDs(unsigned parentID, unsigned depth)
{
    unsigned sideLength = pow(2, depth);
    unsigned parentSideLength = pow(2, depth - 1);
    unsigned parentRows = (parentID - 1) / parentSideLength;
    unsigned basicID = parentID + (parentID - 1);

    glm::vec4 chunksIDs;
    chunksIDs[0] = basicID + 0 + (parentRows + 0) * sideLength;
    chunksIDs[1] = basicID + 1 + (parentRows + 0) * sideLength;
    chunksIDs[2] = basicID + 0 + (parentRows + 1) * sideLength;
    chunksIDs[3] = basicID + 1 + (parentRows + 1) * sideLength;
    return chunksIDs;
}

void DynamicGrid::updateVisibilityState() { }

bool DynamicGrid::isVisible(const Chunk* chunk) { return true; }

void DynamicGrid::getActiveLeafChunks(std::vector<Chunk*>& dest, unsigned depth)
{
    for (Chunk* chunk : visibleLeafChunks[activeTree])
        if (chunk->depth >= depth)
            dest.push_back(chunk);
}

unsigned DynamicGrid::numActiveLeafChunks() { return visibleLeafChunks[activeTree].size(); }

unsigned DynamicGrid::numChunksOrdered()
{
    unsigned count = 0;

    for (auto it = chunks.begin(); it != chunks.end(); it++)
        if (it->second->modelOrdered)
            count++;

    return count;
}

unsigned DynamicGrid::numChunks() { return chunks.size(); }


// TerrainGrid ----------------------------------------------------------------------

TerrainGrid::TerrainGrid(Renderer* renderer, Noiser* noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, bool transparency)
    : DynamicGrid(glm::vec3(0.1f, 0.1f, 0.1f), renderer, 0, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, transparency), noiseGen(noiseGenerator)
{ }

QuadNode<Chunk*>* TerrainGrid::getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID)
{
    if (chunks.find(center) == chunks.end())
        chunks[center] = new PlainChunk(
            *renderer, 
            noiseGen, 
            glm::vec3(std::get<0>(center), std::get<1>(center), std::get<2>(center)), 
            sideLength / (numSideVertex - 1), 
            numSideVertex, 
            numSideVertex, 
            depth,
            chunkID);

    return new QuadNode<Chunk*>(chunks[center]);
}

std::tuple<float, float, float> TerrainGrid::closestCenter()
{
    float maxUsedCellSize = rootCellSize;
    for (size_t level = 0; level < minLevel; level++) maxUsedCellSize /= 2;

    return std::tuple<float, float, float>(
        maxUsedCellSize * std::round(camPos.x / maxUsedCellSize),
        maxUsedCellSize * std::round(camPos.y / maxUsedCellSize),
        0);
}

// PlanetGrid ----------------------------------------------------------------------

PlanetGrid::PlanetGrid(Renderer* renderer, std::shared_ptr<Noiser> noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter, bool transparency)
    : DynamicGrid(glm::vec3(0.1f, 0.1f, 0.1f), renderer, 0, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, transparency), 
    noiseGen(noiseGenerator), radius(radius), nucleus(nucleus), cubePlane(cubePlane), cubeSideCenter(cubeSideCenter) 
{ }

float PlanetGrid::getRadius() { return radius; }

QuadNode<Chunk*>* PlanetGrid::getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID)
{
    if (chunks.find(center) == chunks.end())
        chunks[center] = new PlanetChunk(
            *renderer, 
            noiseGen, 
            glm::vec3(std::get<0>(center), std::get<1>(center), std::get<2>(center)), 
            sideLength / (numSideVertex - 1), 
            numSideVertex, 
            numSideVertex, 
            radius, 
            nucleus, 
            cubePlane, 
            depth,
            chunkID);
    
    return new QuadNode<Chunk*>(chunks[center]);
}

std::tuple<float, float, float> PlanetGrid::closestCenter()
{
    return std::tuple<float, float, float>(cubeSideCenter.x, cubeSideCenter.y, cubeSideCenter.z);
}

void PlanetGrid::updateVisibilityState()
{
    float camDist = glm::length(camPos - nucleus);
    camDist += 0.15 * radius;               // Small addition for ensuring minimal rendering
    if (camDist < radius) { dotHorizon = 1; return; }

    float angle = acos(radius / camDist);
    float relX = cos(angle) * radius;
    float relY = sqrt(radius * radius - relX * relX);   //sin(angle) * radius;

    dotHorizon = glm::dot(
        glm::normalize(glm::vec3(relX, relY, 0)),
        glm::normalize(glm::vec3(camDist, 0, 0)));
}

bool PlanetGrid::isVisible(const Chunk* chunk)
{
    float dotChunk = glm::dot(
        glm::normalize(((PlanetChunk*)chunk)->getGroundCenter()),
        glm::normalize(camPos - nucleus));

    if (dotChunk > dotHorizon) return true;
    else return false;
}

glm::vec3 PlanetGrid::getChunkCenter(Chunk* chunk)
{
    float height = glm::length(camPos) - radius;
    float range  = 200;                                 // if cam is below range height, chunk center is at same height as cam. 
    float ratio  = glm::clamp(height/range, 0.f, 1.f);

    return chunk->getGeoideCenter() + glm::normalize(chunk->getGeoideCenter()) * range * ratio;
}

// SphereGrid ------------------------------------------------------------------

SphereGrid::SphereGrid(Renderer* renderer, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter, bool transparency)
    : PlanetGrid::PlanetGrid(renderer, nullptr, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, cubePlane, cubeSideCenter, transparency)
{ }

QuadNode<Chunk*>* SphereGrid::getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID)
{
    if (chunks.find(center) == chunks.end())    // if chunk was not created previously
        chunks[center] = new SphereChunk(
            *renderer,
            glm::vec3(std::get<0>(center), std::get<1>(center), std::get<2>(center)),
            sideLength / (numSideVertex - 1),
            numSideVertex,
            numSideVertex,
            radius,
            nucleus,
            cubePlane,
            depth,
            chunkID);
    
    return new QuadNode<Chunk*>(chunks[center]);
}

// Planet ----------------------------------------------------------------------

Planet::Planet(Renderer* renderer, std::shared_ptr<Noiser> noiseGenerator, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency)
    : radius(radius), 
    nucleus(nucleus), 
    noiseGen(noiseGenerator),
    readyForUpdate(false)
{
    planetGrid_pZ = new PlanetGrid(renderer, noiseGenerator, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 0,  0,  1), glm::vec3( 0,  0, 50), transparency);
    planetGrid_nZ = new PlanetGrid(renderer, noiseGenerator, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 0,  0, -1), glm::vec3( 0,  0,-50), transparency);
    planetGrid_pY = new PlanetGrid(renderer, noiseGenerator, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 0,  1,  0), glm::vec3( 0, 50,  0), transparency);
    planetGrid_nY = new PlanetGrid(renderer, noiseGenerator, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 0, -1,  0), glm::vec3( 0,-50,  0), transparency);
    planetGrid_pX = new PlanetGrid(renderer, noiseGenerator, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 1,  0,  0), glm::vec3( 50, 0,  0), transparency);
    planetGrid_nX = new PlanetGrid(renderer, noiseGenerator, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3(-1,  0,  0), glm::vec3(-50, 0,  0), transparency);
}

Planet::~Planet()
{ 
    delete planetGrid_pZ;
    delete planetGrid_nZ;
    delete planetGrid_pY;
    delete planetGrid_nY;
    delete planetGrid_pX;
    delete planetGrid_nX;
};

void Planet::addResources(const std::vector<ShaderLoader>& shaders, const std::vector<TextureLoader>& textures)
{
    planetGrid_pZ->addResources(shaders, textures);
    planetGrid_nZ->addResources(shaders, textures);
    planetGrid_pY->addResources(shaders, textures);
    planetGrid_nY->addResources(shaders, textures);
    planetGrid_pX->addResources(shaders, textures);
    planetGrid_nX->addResources(shaders, textures);

    readyForUpdate = true;
}

void Planet::updateState(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj, const LightSet& lights, float frameTime, float groundHeight)
{
    if (readyForUpdate)
    {
        planetGrid_pZ->updateTree(camPos, lights.numLights);
        planetGrid_pZ->updateUBOs(view, proj, camPos, lights, frameTime, groundHeight);
        planetGrid_nZ->updateTree(camPos, lights.numLights);
        planetGrid_nZ->updateUBOs(view, proj, camPos, lights, frameTime, groundHeight);
        planetGrid_pY->updateTree(camPos, lights.numLights);
        planetGrid_pY->updateUBOs(view, proj, camPos, lights, frameTime, groundHeight);
        planetGrid_nY->updateTree(camPos, lights.numLights);
        planetGrid_nY->updateUBOs(view, proj, camPos, lights, frameTime, groundHeight);
        planetGrid_pX->updateTree(camPos, lights.numLights);
        planetGrid_pX->updateUBOs(view, proj, camPos, lights, frameTime, groundHeight);
        planetGrid_nX->updateTree(camPos, lights.numLights);
        planetGrid_nX->updateUBOs(view, proj, camPos, lights, frameTime, groundHeight);
    }
}

void Planet::toLastDraw()
{
    planetGrid_pZ->toLastDraw();
    planetGrid_nZ->toLastDraw();
    planetGrid_pY->toLastDraw();
    planetGrid_nY->toLastDraw();
    planetGrid_pX->toLastDraw();
    planetGrid_nX->toLastDraw();
}

std::shared_ptr<Noiser> Planet::getNoiseGen() const { return noiseGen; }

float Planet::getSphereArea() { return 4 * pi * radius * radius; }

void Planet::printCounts()
{
    unsigned nChunks = planetGrid_pZ->numChunks() + planetGrid_nZ->numChunks() + planetGrid_pY->numChunks() + planetGrid_nY->numChunks() + planetGrid_pX->numChunks() + planetGrid_nX->numChunks();
    unsigned nOrderedChunks = planetGrid_pZ->numChunksOrdered() + planetGrid_nZ->numChunksOrdered() + planetGrid_pY->numChunksOrdered() + planetGrid_nY->numChunksOrdered() + planetGrid_pX->numChunksOrdered() + planetGrid_nX->numChunksOrdered();
    unsigned nActiveLeafChunks = planetGrid_pZ->numActiveLeafChunks() + planetGrid_nZ->numActiveLeafChunks() + planetGrid_pY->numActiveLeafChunks() + planetGrid_nY->numActiveLeafChunks() + planetGrid_pX->numActiveLeafChunks() + planetGrid_nX->numActiveLeafChunks();

    std::cout << "C: " << nChunks << " / OC: " << nOrderedChunks << ", / ALF: " << nActiveLeafChunks << std::endl;
}

float Planet::getGroundHeight(const glm::vec3& camPos)
{
    if (readyForUpdate)
    {
        glm::vec3 ground = glm::normalize(camPos - nucleus) * radius;
        return radius + noiseGen->getNoise(ground.x, ground.y, ground.z);
    }

    return 0;
}

void Planet::getActiveLeafChunks(std::vector<Chunk*>& dest, unsigned depth) const
{
    planetGrid_pZ->getActiveLeafChunks(dest, depth);
    planetGrid_nZ->getActiveLeafChunks(dest, depth);
    planetGrid_pY->getActiveLeafChunks(dest, depth);
    planetGrid_nY->getActiveLeafChunks(dest, depth);
    planetGrid_pX->getActiveLeafChunks(dest, depth);
    planetGrid_nX->getActiveLeafChunks(dest, depth);
}

float Planet::callBack_getFloorHeight(const glm::vec3& pos)
{
    glm::vec3 nucleus(0.f, 0.f, 0.f);
    float radius = planetGrid_pZ->getRadius();
    glm::vec3 espheroid = glm::normalize(pos - nucleus) * radius;
    return 1.70 + radius + noiseGen->getNoise(espheroid.x, espheroid.y, espheroid.z);
}

// Sphere ----------------------------------------------------------------------

Sphere::Sphere(Renderer* renderer, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, bool transparency)
    : Planet(renderer, nullptr, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, transparency)
{
    delete planetGrid_pZ;
    delete planetGrid_nZ;
    delete planetGrid_pY;
    delete planetGrid_nY;
    delete planetGrid_pX;
    delete planetGrid_nX;

    planetGrid_pZ = new SphereGrid(renderer, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 0, 0, 1), glm::vec3(  0,  0, 50), transparency);
    planetGrid_nZ = new SphereGrid(renderer, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 0, 0,-1), glm::vec3(  0,  0,-50), transparency);
    planetGrid_pY = new SphereGrid(renderer, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 0, 1, 0), glm::vec3(  0, 50,  0), transparency);
    planetGrid_nY = new SphereGrid(renderer, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 0,-1, 0), glm::vec3(  0,-50,  0), transparency);
    planetGrid_pX = new SphereGrid(renderer, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3( 1, 0, 0), glm::vec3( 50,  0,  0), transparency);
    planetGrid_nX = new SphereGrid(renderer, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3(-1, 0, 0), glm::vec3(-50,  0,  0), transparency);
}

Sphere::~Sphere()
{
    // The 6 grids are already deleted in Planet::~Planet()
};

float Sphere::callBack_getFloorHeight(const glm::vec3& pos)
{
    glm::vec3 nucleus(0.f, 0.f, 0.f);
    float radius = planetGrid_pZ->getRadius();
    return 1.70 + radius;
}


// Grass ----------------------------------------------------------------------

GrassSystem::GrassSystem(Renderer& renderer, float maxDist, bool(*grassSupported_callback)(const glm::vec3& pos, float groundSlope))
    : renderer(renderer), modelOrdered(false), camPos(1,2,3), camDir(1,2,3), fov(0), pi(3.141592653589793238462), maxDist(maxDist), grassSupported(grassSupported_callback)
{ }

GrassSystem::~GrassSystem()
{
    // More info in Chunk::~Chunk()
    //if (renderer.getModelsCount() && modelOrdered)
    //    renderer.deleteModel(grassModel);
}

void GrassSystem::createGrassModel(std::vector<ShaderLoader>& shaders, std::vector<TextureLoader>& textures, const LightSet* lights)
{
    float hor = 1;
    float ver = 1;
    float vMove = 0.1;
    std::vector<float> vertices =       // plane XY (pos, normal, UV) centered at X axis
    { 
         0   - ver * vMove,  hor/2, 0,   1, 0, 0,   0, 0,     //     y|___
         0   - ver * vMove, -hor/2, 0,   1, 0, 0,   1, 0,     //   ___|   |___x
         ver - ver * vMove, -hor/2, 0,   1, 0, 0,   1, 1,     //      |___|
         ver - ver * vMove,  hor/2, 0,   1, 0, 0,   0, 1      //      |
    };

    std::vector<uint16_t> indices = { 0, 1, 2,  0, 2, 3 };
    
    VerticesLoader vertexData(vt_332.vertexSize, vertices.data(), 8, indices);
    //std::vector<ShaderLoader> shaders{ ShaderLoaders[8], ShaderLoaders[9] };
    //std::vector<TextureLoader> textures{ texInfos[37] };

    grassModel = renderer.newModel(
        "grass",
        1, 5, primitiveTopology::triangle, vt_332,
        vertexData, shaders, textures,
        5, 4 * size.mat4 + 2 * size.vec4 + lights->posDirBytes,     // M, V, P, MN, camPos + time, centerPos, lights
        lights->propsBytes,                                         // lights, centerPos
        false,
        0,
        VK_CULL_MODE_NONE);
    
    modelOrdered = true;
}

void GrassSystem::toLastDraw() 
{ 
    if(modelOrdered && grassModel->fullyConstructed)
        renderer.toLastDraw(grassModel); 
}

bool GrassSystem::withinFOV(const glm::vec3& itemPos, const glm::vec3& camPos, const glm::vec3& camDir, float fov)
{
    /* Readable version
    glm::vec3 itemDir = glm::normalize(itemPos - camPos);
    float camDirSide = glm::dot(itemDir, camDir);
    float angle = acos(camDirSide);

    if (angle > fov) return false;
    else return true;
    */
    if (acos(glm::dot(glm::normalize(itemPos - camPos), camDir)) > fov) return false;
    return true;
}


GrassSystem_XY::GrassSystem_XY(Renderer& renderer, float step, float side, float maxDist)
    : GrassSystem(renderer, maxDist), step(step), side(side)
{
    std::mt19937_64 engine(38572);
    std::uniform_real_distribution<float> distributor(0, 2 * pi);

    for (int i = 0; i < 30; i++)
        for (int j = 0; j < 30; j++)
            whiteNoise[i][j] = distributor(engine);
}

GrassSystem_XY::~GrassSystem_XY() { }

void GrassSystem_XY::getGrassItems()
{
    glm::vec3 gPos;     // grass bouquet position
    glm::ivec2 nIdx;    // noise index
    float sqrDist;
    glm::vec3 distVec;

    pos.clear();
    rot.clear();
    sca.clear();

    glm::vec3 pos0 = { step * round(camPos.x / step) - step * side/2,   step * round(camPos.y / step) - step * side/2,   0 };

    for (int i = 0; i < side + 1; i++)
        for (int j = 0; j < side + 1; j++)
        {
            // Position
            gPos.x = pos0.x + i * step;
            gPos.y = pos0.y + j * step;
            gPos.z = 0.5;

            distVec = gPos - glm::vec3(camPos.x, camPos.y, 0);
            sqrDist = distVec.x * distVec.x + distVec.y * distVec.y + distVec.z * distVec.z;
            if (sqrDist > maxDist* maxDist) continue;

            pos.push_back(gPos);

            // Rotation (noise)
            nIdx.x = unsigned(round(abs(gPos.x / step))) % 30U;
            nIdx.y = unsigned(round(abs(gPos.y / step))) % 30U;
            rot.push_back(getRotQuat(zAxis, whiteNoise[nIdx.x][nIdx.y]));
            //rot.push_back(glm::vec3(0, 0, whiteNoise[nIdx.x][nIdx.y]));

            // Rotation (lookAt camPos)
            //rot.push_back(glm::vec3(0, 0, 2*pi - atan((gPos.x - camPos.x) / (gPos.y - camPos.y))));

            // Scaling
            sca.push_back(glm::vec3(1, 1, 1));
        }

    //if(toSort) sorter.sort(pos, index, camPos, 0, pos.size() - 1);
}

void GrassSystem_planet::updateState(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camDir, float fov, const LightSet& lights, const Planet& planet, float time)
{
    if (!modelOrdered) return;
    
    // Get grass parameters
    if (this->camDir != camDir || this->camPos != camPos || this->fov != fov || renderRequired(planet))
    {
        this->camDir = camDir;
        this->camPos = camPos;
        this->fov = fov;
        getGrassItems(planet);
        renderer.setRenders(grassModel, pos.size());
    }

    // Update UBOs
    uint8_t* dest;
    float rotation;
    glm::mat4 model, modelNormals;

    for (int i = 0; i < pos.size(); i++)
    {
        model = getModelMatrix(sca[i], rot[i], pos[i]);
        modelNormals = getModelMatrixForNormals(model);

        dest = grassModel->vsDynUBO.getUBOptr(i);

        memcpy(dest, &model, size.mat4);
        dest += size.mat4;
        memcpy(dest, &view, size.mat4);
        dest += size.mat4;
        memcpy(dest, &proj, size.mat4);
        dest += size.mat4;
        memcpy(dest, &modelNormals, size.mat4);
        dest += size.mat4;
        memcpy(dest, &camPos, size.vec3);
        dest += size.vec3;
        memcpy(dest, &time, sizeof(float));
        dest += sizeof(float);
        memcpy(dest, &pos[i], size.vec3);
        dest += size.vec3;
        memcpy(dest, &slp[i], sizeof(float));
        dest += sizeof(float);
        memcpy(dest, lights.posDir, lights.posDirBytes);
    }

    dest = grassModel->fsUBO.getUBOptr(0);
    memcpy(dest, lights.props, lights.propsBytes);
}

GrassSystem_planet::GrassSystem_planet(Renderer& renderer, float maxDist, unsigned minDepth)
    : GrassSystem(renderer, maxDist), minDepth(minDepth), chunksCount(0)
{
    std::mt19937_64 engine(38572);
    std::uniform_real_distribution<float> distributor(0, 2 * pi);

    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++)
            for (int k = 0; k < 15; k++)
                whiteNoise[i][j][k] = distributor(engine);
}

GrassSystem_planet::~GrassSystem_planet() { }

void GrassSystem_planet::getGrassItems(const Planet& planet)
{
    //getGrassItems_fullGrass(planet, toSort);
    getGrassItems_average(planet);

    if (pos.size() > maxPosSize) maxPosSize = pos.size();

    //std::cout << "Grass items: " << maxPosSize << " / " << pos.size() << std::endl;
}

void GrassSystem_planet::getGrassItems_average(const Planet& planet)
{
    // Reserved memory variables
    glm::vec3 gPos;     // grass bunch position
    glm::ivec3 nIdx;    // noise indices
    float distance;
    std::vector<float>* vert;
    glm::vec3 dirToCam, dirToCamXY;
    float angle, groundSlope;
    glm::vec4 finalQuat;

    // Precalculations
    float step = 0.5;   // <<< this is arbitrary
    glm::vec3 normal = glm::normalize(camPos - planet.nucleus);
    glm::vec4 latLonQuat = getLatLonRotQuat(normal);
    glm::vec3 front = rotatePoint(latLonQuat, zAxis);
    glm::vec3 right = glm::normalize(glm::cross(front, normal));     //glm::normalize(glm::cross(front, zAxis));

    // Get vertices for grass
    chunks.clear();
    planet.getActiveLeafChunks(chunks, minDepth);
    chunksCount = chunks.size();

    // Fill parameters for each grass bunch
    pos.clear();
    rot.clear();
    sca.clear();
    slp.clear();

    for (int i = 0; i < chunks.size(); i++)
    {
        vert = chunks[i]->getVertices();

        for (int j = 0; j < vert->size(); j += 9)
        {
            // Filter (choose positions that support grass)
            gPos = glm::vec3((*vert)[j + 0], (*vert)[j + 1], (*vert)[j + 2]);
            groundSlope = 1.f - glm::dot(glm::vec3((*vert)[j + 3], (*vert)[j + 4], (*vert)[j + 5]), normal);    // dot(groundNormal, sphereNormal)

            if (!grassSupported(gPos, groundSlope) ||           // user conditions
                !withinFOV(gPos, camPos, camDir, fov * 1.05)    // is outside fov?
                ) continue;

            // Get grass parameters (pos, rot, scl, slp, index)
            
            // > [Rotations]
            distance = getDist(gPos, camPos);

            if (distance < maxDist)
            {
                if (((int)gPos.x % 2 || (int)gPos.y % 2 || (int)gPos.z % 2) && groundSlope > 0.05) continue;

                // Random (noise)
                nIdx.x = unsigned(round(abs(gPos.x / step))) % 15U;
                nIdx.y = unsigned(round(abs(gPos.y / step))) % 15U;
                nIdx.z = unsigned(round(abs(gPos.z / step))) % 15U;
                finalQuat = productQuat(getRotQuat(xAxis, whiteNoise[nIdx.x][nIdx.y][nIdx.z]), latLonQuat);

                // Ommit too sided billboards
                //if (std::abs(glm::dot(rotatePoint(finalQuat, zAxis), glm::normalize(camPos - gPos))) < 0.3) continue;
            }
            else
            {
                if ((int)gPos.x % 2 || (int)gPos.y % 2 || (int)gPos.z % 2) continue;

                // Face camPos
                dirToCam = glm::normalize(camPos - gPos);
                dirToCamXY = getProjectionOnPlane(normal, dirToCam);
                angle = angleBetween(front, dirToCamXY);
                if (glm::dot(dirToCam, right) > 0) angle *= -1;
                finalQuat = productQuat(getRotQuat(xAxis, angle), latLonQuat);
            }

            rot.push_back(finalQuat);

            // > [Position]
            pos.push_back(gPos);

            // > [Slope]
            slp.push_back(groundSlope);

            // > [Scaling]
            sca.push_back(glm::vec3(1.5, 1.5, 1.5));    // <<<

            // Add double billboard (bb) to near grass (crossed bbs)
            if (distance < maxDist * 0.5)
            {
                pos.push_back(gPos);
                slp.push_back(groundSlope);
                rot.push_back(productQuat(getRotQuat(xAxis, whiteNoise[nIdx.x][nIdx.y][nIdx.z] + pi / 2), latLonQuat));
                sca.push_back(glm::vec3(1.5, 1.5, 1.5));
            }
        }
    }

    // > [Index]
    //index.resize(pos.size());
    //for (int i = 0; i < pos.size(); i++) index[i] = i;
    //if(toSort) sorter.sort(pos, index, camPos, 0, pos.size() - 1);
}

void GrassSystem_planet::getGrassItems_fullGrass(const Planet& planet)
{
    // Reserved memory variables
    glm::vec3 gPos;     // grass bunch position
    glm::ivec3 nIdx;    // noise indices
    float distance;
    std::vector<float>* vert;
    glm::vec3 dirToCam, dirToCamXY;
    float angle, groundSlope;
    glm::vec4 finalQuat;

    // Precalculations
    float step           = 0.5;   // <<< this is arbitrary
    glm::vec3 normal     = glm::normalize(camPos - planet.nucleus);     // Cam's normal is considered the normal for all items.
    glm::vec4 latLonQuat = getLatLonRotQuat(normal);                    // Rotation quaternion around world coordinates for all items.
    glm::vec3 front      = rotatePoint(latLonQuat, zAxis);              // Front for all items (looks north).
    glm::vec3 right      = glm::normalize(glm::cross(front, normal));   // Right for all items. // glm::normalize(glm::cross(front, zAxis));
    
    // Get vertices for grass
    chunks.clear();
    planet.getActiveLeafChunks(chunks, minDepth);
    chunksCount = chunks.size();

    // Fill parameters for each grass bunch
    pos.clear();
    rot.clear();
    sca.clear();
    slp.clear();
    
    for (int i = 0; i < chunks.size(); i++)     // for each chunk
    {
        vert = chunks[i]->getVertices();

        for (int j = 0; j < vert->size(); j += 9)   // for each vertex
        {
            // Filter (choose positions that support grass)
            gPos = glm::vec3((*vert)[j+0], (*vert)[j+1], (*vert)[j+2]);
            groundSlope = 1.f - glm::dot(glm::vec3((*vert)[j + 3], (*vert)[j + 4], (*vert)[j + 5]),  // ground normal
                                         normal);                                                    // sphere normal

            if( !grassSupported(gPos, groundSlope) ||           // user conditions
                !withinFOV(gPos, camPos, camDir, fov * 1.05)    // is outside fov?
            ) continue;

            // Filter most far grass pseudo-randomly
            distance = getDist(gPos, camPos);
            if (distance > maxDist)
                if ((int)gPos.x % 2 || (int)gPos.y % 2 || (int)gPos.z % 2) continue;

            // Get grass parameters (pos, rot, scl, slp, index)
            // > [Position]
            pos.push_back(gPos);

            // > [Slope]
            slp.push_back(groundSlope);

            // > [Rotations]
            if (distance < maxDist)     // Random (noise)
            {
                nIdx.x = unsigned(round(abs(gPos.x / step))) % 15U;
                nIdx.y = unsigned(round(abs(gPos.y / step))) % 15U;
                nIdx.z = unsigned(round(abs(gPos.z / step))) % 15U;
                finalQuat = productQuat(getRotQuat(xAxis, whiteNoise[nIdx.x][nIdx.y][nIdx.z]), latLonQuat);
            }
            else    // Face camPos
            {
                dirToCam = glm::normalize(camPos - gPos);
                dirToCamXY = getProjectionOnPlane(normal, dirToCam);
                angle = angleBetween(front, dirToCamXY);
                if (glm::dot(dirToCam, right) > 0) angle *= -1;
                finalQuat = productQuat(getRotQuat(xAxis, angle), latLonQuat);
            }

            rot.push_back(finalQuat);

            // > [Scaling]
            sca.push_back(glm::vec3(1.5, 1.5, 1.5));    // <<<

            // Add double billboard (bb) to near grass (crossed bbs)
            if (distance < maxDist * 0.2)
            {
                pos.push_back(gPos);
                slp.push_back(groundSlope);
                rot.push_back(productQuat(getRotQuat(xAxis, whiteNoise[nIdx.x][nIdx.y][nIdx.z] + pi/2), latLonQuat));
                sca.push_back(glm::vec3(1.5, 1.5, 1.5));
            }
        }
    }

    // > [Index]
    //index.resize(pos.size());
    //for (int i = 0; i < pos.size(); i++) index[i] = i;
    //if(toSort) sorter.sort(pos, index, camPos, 0, pos.size() - 1);
}

glm::vec4 GrassSystem_planet::getLatLonRotQuat(glm::vec3& normal)
{
    glm::vec3 normalXY = { normal.x, normal.y, 0 };

    float longitude = angleBetween(xAxis, normalXY);
    if (normalXY.y < 0) longitude *= -1.f;

    float latitude = angleBetween(normalXY, normal);
    if (normal.z < 0) latitude *= -1.f;

    return productQuat(
        getRotQuat(yAxis, -latitude),
        getRotQuat(zAxis,  longitude) );
}

glm::vec3 GrassSystem_planet::getProjectionOnPlane(glm::vec3& normal, glm::vec3& vec)
{
    // B × (X × Y) / ||(X × Y)||^2

    // Proj(on Normal) = ((Vec · Normal) / ||Normal||^2) Normal
    // Vec = Proj(on Plane) + Proj(on Normal)
    // Proj(on Plane) = Vec - Proj(on Normal)

    glm::vec3 projOnNormal = glm::dot(vec, normal) * normal;
    return vec - projOnNormal;
}

bool GrassSystem_planet::renderRequired(const Planet& planet)
{
    std::vector<Chunk*> availableChunks;
    planet.getActiveLeafChunks(availableChunks, minDepth);

    if (availableChunks.size() == chunksCount) return false;
    else return true;
}

bool grassSupported_callback(const glm::vec3& pos, float groundSlope)
{
    float height = glm::distance(pos, zero);
    if (groundSlope > 0.22 || height < 2014 || height > 2090) return false;

    return true;
}
