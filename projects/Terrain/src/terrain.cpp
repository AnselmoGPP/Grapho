#include "terrain.hpp"
#include "ubo.hpp"


// Chunk ----------------------------------------------------------------------

Chunk::Chunk(Renderer& renderer, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned depth, unsigned chunkID)
    : renderer(renderer), stride(stride), numHorVertex(numHorVertex), numVertVertex(numVertVertex), numAttribs(9), depth(depth), modelOrdered(false), chunkID(chunkID)
{
    baseCenter = center;
    groundCenter = baseCenter;
}

Chunk::~Chunk()
{
    if (renderer.getModelsCount() && modelOrdered)
        renderer.deleteModel(model);
}

void Chunk::render(ShaderIter vertexShader, ShaderIter fragmentShader, std::vector<texIterator>& usedTextures, std::vector<uint16_t>* indices)
{
    VertexLoader* vertexLoader = new VertexFromUser(
        VertexType({ vec3size, vec3size, vec3size }, { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT }),
        numHorVertex * numVertVertex,
        vertex.data(),
        indices ? *indices : this->indices,
        true);

    model = renderer.newModel(
        "chunk",
        1, 1, primitiveTopology::triangle,
        vertexLoader,
        1, 4 * mat4size + 2 * vec4size + 2 * sizeof(LightPosDir),   // MM (mat4), VM (mat4), PM (mat4), MMN (mat3), camPos (vec3), n * LightPosDir (2*vec4), sideDepth (vec3)
        vec4size + 2 * sizeof(LightProps),                          // Time (float), n * LightProps (6*vec4)
        usedTextures,
        vertexShader, fragmentShader,
        false);

    uint8_t* dest;
    for (size_t i = 0; i < model->vsDynUBO.numDynUBOs; i++)
    {
        dest = model->vsDynUBO.getUBOptr(i);
        memcpy(dest, &modelMatrix(), mat4size);
        dest += mat4size;
        //memcpy(dest, &view, mat4size);
        dest += mat4size;
        //memcpy(dest, &proj, mat4size);
        dest += mat4size;
        memcpy(dest, &modelMatrixForNormals(modelMatrix()), mat4size);
        dest += mat4size;
        //memcpy(dest, &camPos, vec3size);
        //dest += vec4size;
        //memcpy(dest, &sideDepths, vec4size);
        //dest += vec4size;
        //memcpy(dest, lights.posDir, lights.posDirBytes);
        //dest += lights.posDirBytes;
    }

    //dest = model->fsUBO.getUBOptr(0);
    //memcpy(dest, &time, sizeof(time));
    //dest += vec4size;
    //memcpy(dest, lights.props, lights.propsBytes);
    //dest += lights.propsBytes;

    modelOrdered = true;
}

void Chunk::updateUBOs(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos, LightSet& lights, float time, glm::vec3 planetCenter)
{
    if (!modelOrdered) return;

    uint8_t* dest;
    for (size_t i = 0; i < model->vsDynUBO.numDynUBOs; i++)
    {
        dest = model->vsDynUBO.getUBOptr(i);
        //memcpy(dest, &modelMatrix(), mat4size);
        dest += mat4size;
        memcpy(dest, &view, mat4size);
        dest += mat4size;
        memcpy(dest, &proj, mat4size);
        dest += mat4size;
        //memcpy(dest, &modelMatrixForNormals(modelMatrix()), mat4size);
        dest += mat4size;
        memcpy(dest, &camPos, vec3size);
        dest += vec4size;
        memcpy(dest, &sideDepths, vec4size);
        dest += vec4size;
        memcpy(dest, lights.posDir, lights.posDirBytes);
        //dest += lights.posDirBytes;
    }

    dest = model->fsUBO.getUBOptr(0);
    memcpy(dest, &time, sizeof(time));
    dest += vec4size;
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

// PlainChunk ----------------------------------------------------------------------

PlainChunk::PlainChunk(Renderer& renderer, Noiser* noiseGenerator, glm::vec3 center, float stride, unsigned numHorVertex, unsigned numVertVertex, unsigned depth, unsigned chunkID)
    : Chunk(renderer, center, stride, numHorVertex, numVertVertex, depth, chunkID), noiseGen(noiseGenerator)
{
    groundCenter.z = noiseGen->GetNoise(baseCenter.x, baseCenter.y);

    computeSizes();
}

void PlainChunk::computeTerrain(bool computeIndices, float textureFactor)   // <<< Fix function to make it similar to SphericalChunk::computeTerrain()
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

            // positions (0, 1, 2)
            vertex[index * 6 + 0] = x0 + x * stride;
            vertex[index * 6 + 1] = y0 + y * stride;
            vertex[index * 6 + 2] = noiseGen->GetNoise((float)vertex[index * 6 + 0], (float)vertex[index * 6 + 1]);
            //std::cout << vertex[pos * 8 + 0] << ", " << vertex[pos * 8 + 1] << ", " << vertex[pos * 8 + 2] << std::endl;

            // textures (3, 4)
            //vertex[index * 8 + 3] = vertex[index * 8 + 0] * textureFactor;
            //vertex[index * 8 + 4] = vertex[index * 8 + 1] * textureFactor;     // LOOK produces textures reflected in the x-axis
            //std::cout << vertex[pos * 8 + 3] << ", " << vertex[pos * 8 + 4] << std::endl;
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

PlanetChunk::PlanetChunk(Renderer& renderer, Noiser* noiseGenerator, glm::vec3 cubeSideCenter, float stride, unsigned numHorVertex, unsigned numVertVertex, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, unsigned depth, unsigned chunkID)
    : Chunk(renderer, cubeSideCenter, stride, numHorVertex, numVertVertex, depth, chunkID), noiseGen(noiseGenerator), nucleus(nucleus), radius(radius)
{
    glm::vec3 unitVec = glm::normalize(baseCenter - nucleus);
    glm::vec3 sphere = unitVec * radius;
    groundCenter = sphere + unitVec * noiseGen->GetNoise(sphere.x, sphere.y, sphere.z);

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

void PlanetChunk::computeTerrain(bool computeIndices, float textureFactor)
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
            ground = sphere + unitVec * noiseGen->GetNoise(sphere.x, sphere.y, sphere.z);
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

// DynamicGrid ----------------------------------------------------------------------

DynamicGrid::DynamicGrid(glm::vec3 camPos, LightSet& lights, Renderer& renderer, unsigned activeTree, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier)
    : camPos(camPos), lights(&lights), renderer(renderer), activeTree(activeTree), loadedChunks(0), rootCellSize(rootCellSize), numSideVertex(numSideVertex), numLevels(numLevels), minLevel(minLevel), distMultiplier(distMultiplier)
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

void DynamicGrid::addTextures(const std::vector<texIterator>& textures) { this->textures = textures; }

void DynamicGrid::addShaders(ShaderIter vertexShader, ShaderIter fragmentShader)
{
    this->vertShader = vertexShader;
    this->fragShader = fragmentShader;
}

void DynamicGrid::updateTree(glm::vec3 newCamPos)
{
    if (!numLevels) return;

    // Return if CamPos has not changed, Active tree exists, and Non-active tree is nullptr.
    unsigned nonActiveTree = (activeTree + 1) % 2;
    if (root[activeTree] && !root[nonActiveTree] && camPos.x == newCamPos.x && camPos.y == newCamPos.y && camPos.z == newCamPos.z)
        return;  // ERROR: When updateTree doesn't run in each frame (i.e., when command buffer isn't created each frame), no validation error appears after resizing window

    // Build tree if the non-active tree is nullptr
    if (!root[nonActiveTree])
    {
        camPos = newCamPos;

        std::tuple<float, float, float> center = closestCenter();
        root[nonActiveTree] = getNode(center, rootCellSize, 0, 1);

        renderedChunks = 0;
        createTree(root[nonActiveTree], 0);                         // Build tree and load leaf-chunks
    }

    // <<< Why is executed each time I move? Why is only executed in one plane for fixing gaps?
    // Check whether non-active tree has fully constructed leaf-chunks. If so, switch trees
    if (fullConstChunks(root[nonActiveTree]))   // <<< Can this process be improved by setting a flag when tree is fully constructed?
    {
        updateChunksSideDepths(root[nonActiveTree]);

        changeRenders(root[activeTree], false);
        if (root[activeTree]) delete root[activeTree];
        root[activeTree] = nullptr;

        changeRenders(root[nonActiveTree], true);
        activeTree = nonActiveTree;

        removeFarChunks(3, newCamPos);
    }
}

void DynamicGrid::createTree(QuadNode<Chunk*>* node, size_t depth)
{
    Chunk* chunk = node->getElement();
    glm::vec3 gCenter = chunk->getGroundCenter();
    float chunkLength = chunk->getHorChunkSide();
    float sqrSide = chunkLength * chunkLength;
    float sqrDist = (camPos.x - gCenter.x) * (camPos.x - gCenter.x) + (camPos.y - gCenter.y) * (camPos.y - gCenter.y) + (camPos.z - gCenter.z) * (camPos.z - gCenter.z);
    
    //std::cout << node->getElement()->chunkID << std::endl;

    // Is leaf node > Compute terrain > Children are nullptr by default
    if (depth >= minLevel && (sqrDist > sqrSide * distMultiplier || depth == numLevels - 1))
    {
        renderedChunks++;
        if (chunk->modelOrdered == false)
        {
            chunk->computeTerrain(false);       //, std::pow(2, numLevels - 1 - depth));
            chunk->render(vertShader, fragShader, textures, &indices);
            //chunk->updateUBOs(camPos, view, proj);
            renderer.setRenders(chunk->model, 0);
            loadedChunks++;
        }
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

void DynamicGrid::updateUBOs(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos, LightSet& lights, float time)
{
    this->view = view;
    this->proj = proj;
    this->camPos = camPos;
    this->lights = &lights;
    this->time = time;

    //preorder<Chunk*, void (QuadNode<Chunk*>*)>(root, nodeVisitor);
    updateUBOs_help(root[activeTree]);  // Preorder traversal
}

void DynamicGrid::updateUBOs_help(QuadNode<Chunk*>* node)
{
    if (!node) return;

    if (node->isLeaf())
    {
        node->getElement()->updateUBOs(view, proj, camPos, *lights, time);
        return;
    }
    else
    {
        updateUBOs_help(node->getA());
        updateUBOs_help(node->getB());
        updateUBOs_help(node->getC());
        updateUBOs_help(node->getD());
    }
}

bool DynamicGrid::fullConstChunks(QuadNode<Chunk*>* node)
{
    if (!node) return false;

    if (node->isLeaf())
    {
        if (node->getElement()->model->fullyConstructed)
            return true;
        else
            return false;
    }
    else
    {
        char full[4];
        full[0] = fullConstChunks(node->getA());
        full[1] = fullConstChunks(node->getB());
        full[2] = fullConstChunks(node->getC());
        full[3] = fullConstChunks(node->getD());

        return full[0] && full[1] && full[2] && full[3];
    }
}

void DynamicGrid::changeRenders(QuadNode<Chunk*>* node, bool renderMode)
{
    if (!node) return;

    if (node->isLeaf())
        renderer.setRenders(node->getElement()->model, (renderMode ? 1 : 0));
    else
    {
        changeRenders(node->getA(), renderMode);
        changeRenders(node->getB(), renderMode);
        changeRenders(node->getC(), renderMode);
        changeRenders(node->getD(), renderMode);
    }
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
        if(currentNode->isLeaf()) 
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

        //uint8_t* ubo = it->model->vsDynUBO.getUBOptr(0);
        //ubo += 4 * mat4size + vec4size;
        //memcpy(ubo, &it->sideDepths, vec4size);
    }
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

void DynamicGrid::removeFarChunks(unsigned relDist, glm::vec3 camPosNow)
{
    glm::vec3 center;
    glm::vec3 distVec;
    float targetDist;
    float targetSqrDist;
    std::map<std::tuple<float, float, float>, Chunk*>::iterator it = chunks.begin();
    std::map<std::tuple<float, float, float>, Chunk*>::iterator nextIt;

    while (it != chunks.end())
    {
        nextIt = it;
        nextIt++;
        targetDist = it->second->getHorChunkSide() * relDist;

        if (it->second->modelOrdered && it->second->model->fullyConstructed && !it->second->model->activeRenders)
        {
            center = it->second->getGroundCenter();
            distVec.x = center.x - camPosNow.x;
            distVec.y = center.y - camPosNow.y;
            distVec.z = center.z - camPosNow.z;
            targetSqrDist = targetDist * targetDist;

            if ((distVec.x * distVec.x + distVec.y * distVec.y + distVec.z * distVec.z) > targetSqrDist)
            {
                delete chunks[it->first];
                chunks.erase(it->first);
                loadedChunks--;
            }
        }

        it = nextIt;
    }
}

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

//void updateUBOs_visitor(QuadNode<PlainChunk*>* node, const TerrainGrid& terrGrid)
//{
//    if (node->isLeaf())
//        node->getElement()->updateUBOs(terrGrid.camPos, terrGrid.view, terrGrid.proj);
//}


// TerrainGrid ----------------------------------------------------------------------

TerrainGrid::TerrainGrid(Renderer& renderer, Noiser* noiseGenerator, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier)
    : DynamicGrid(glm::vec3(0.1f, 0.1f, 0.1f), lights, renderer, 0, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier), noiseGen(noiseGenerator)
{ }

QuadNode<Chunk*>* TerrainGrid::getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID)
{
    if (chunks.find(center) == chunks.end())
        chunks[center] = new PlainChunk(
            renderer, 
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

PlanetGrid::PlanetGrid(Renderer& renderer, Noiser* noiseGenerator, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus, glm::vec3 cubePlane, glm::vec3 cubeSideCenter)
    : DynamicGrid(glm::vec3(0.1f, 0.1f, 0.1f), lights, renderer, 0, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier), noiseGen(noiseGenerator), radius(radius), nucleus(nucleus), cubePlane(cubePlane), cubeSideCenter(cubeSideCenter) 
{ }

float PlanetGrid::getRadius() { return radius; }

QuadNode<Chunk*>* PlanetGrid::getNode(std::tuple<float, float, float> center, float sideLength, unsigned depth, unsigned chunkID)
{
    if (chunks.find(center) == chunks.end())
        chunks[center] = new PlanetChunk(
            renderer, 
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


// Planet ----------------------------------------------------------------------

Planet::Planet(Renderer& renderer, Noiser* noiseGenerator, LightSet& lights, size_t rootCellSize, size_t numSideVertex, size_t numLevels, size_t minLevel, float distMultiplier, float radius, glm::vec3 nucleus)
    : radius(radius), 
    nucleus(nucleus), 
    noiseGen(noiseGenerator), 
    readyForUpdate(false),
    planetGrid_pZ(renderer, noiseGenerator, lights, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3(0, 0, 1), glm::vec3(0, 0, 50)),
    planetGrid_nZ(renderer, noiseGenerator, lights, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3(0, 0, -1), glm::vec3(0, 0, -50)),
    planetGrid_pY(renderer, noiseGenerator, lights, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3(0, 1, 0), glm::vec3(0, 50, 0)),
    planetGrid_nY(renderer, noiseGenerator, lights, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3(0, -1, 0), glm::vec3(0, -50, 0)),
    planetGrid_pX(renderer, noiseGenerator, lights, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3(1, 0, 0), glm::vec3(50, 0, 0)),
    planetGrid_nX(renderer, noiseGenerator, lights, rootCellSize, numSideVertex, numLevels, minLevel, distMultiplier, radius, nucleus, glm::vec3(-1, 0, 0), glm::vec3(-50, 0, 0))
{ }

void Planet::addResources(const std::vector<texIterator>& textures, ShaderIter vertexShader, ShaderIter fragmentShader)
{
    planetGrid_pZ.addTextures(textures);
    planetGrid_nZ.addTextures(textures);
    planetGrid_pY.addTextures(textures);
    planetGrid_nY.addTextures(textures);
    planetGrid_pX.addTextures(textures);
    planetGrid_nX.addTextures(textures);

    planetGrid_pZ.addShaders(vertexShader, fragmentShader);
    planetGrid_nZ.addShaders(vertexShader, fragmentShader);
    planetGrid_pY.addShaders(vertexShader, fragmentShader);
    planetGrid_nY.addShaders(vertexShader, fragmentShader);
    planetGrid_pX.addShaders(vertexShader, fragmentShader);
    planetGrid_nX.addShaders(vertexShader, fragmentShader);

    readyForUpdate = true;
}

void Planet::updateState(const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& proj, LightSet& lights, float frameTime)
{
    if (readyForUpdate)
    {
        planetGrid_pZ.updateTree(camPos);
        planetGrid_pZ.updateUBOs(view, proj, camPos, lights, frameTime);
        planetGrid_nZ.updateTree(camPos);
        planetGrid_nZ.updateUBOs(view, proj, camPos, lights, frameTime);
        planetGrid_pY.updateTree(camPos);
        planetGrid_pY.updateUBOs(view, proj, camPos, lights, frameTime);
        planetGrid_nY.updateTree(camPos);
        planetGrid_nY.updateUBOs(view, proj, camPos, lights, frameTime);
        planetGrid_pX.updateTree(camPos);
        planetGrid_pX.updateUBOs(view, proj, camPos, lights, frameTime);
        planetGrid_nX.updateTree(camPos);
        planetGrid_nX.updateUBOs(view, proj, camPos, lights, frameTime);
    }
}

float Planet::getSphereArea() { return 4 * pi * radius * radius; }

float Planet::callBack_getFloorHeight(const glm::vec3& pos)
{
    glm::vec3 nucleus(0.f, 0.f, 0.f);
    float radius = planetGrid_pZ.getRadius();
    glm::vec3 espheroid = glm::normalize(pos - nucleus) * radius;
    return 1.70 + radius + noiseGen->GetNoise(espheroid.x, espheroid.y, espheroid.z);
}